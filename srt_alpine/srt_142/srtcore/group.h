/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2020 Haivision Systems Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

/*****************************************************************************
Written by
   Haivision Systems Inc.
*****************************************************************************/

#ifndef INC_SRT_GROUP_H
#define INC_SRT_GROUP_H

#include "srt.h"
#include "common.h"
#include "packet.h"

#if ENABLE_HEAVY_LOGGING
const char* const srt_log_grp_state[] = {"PENDING", "IDLE", "RUNNING", "BROKEN"};
#endif

class CUDTGroup
{
    friend class CUDTUnited;

    typedef srt::sync::steady_clock::time_point time_point;
    typedef srt::sync::steady_clock::duration   duration;
    typedef srt::sync::steady_clock             steady_clock;

public:
    typedef SRT_MEMBERSTATUS GroupState;

    // Note that the use of states may differ in particular group types:
    //
    // Broadcast: links that are freshly connected become PENDING and then IDLE only
    // for a short moment to be activated immediately at the nearest sending operation.
    //
    // Balancing: like with broadcast, just that the link activation gets its shared percentage
    // of traffic balancing
    //
    // Multicast: The link is never idle. The data are always sent over the UDP multicast link
    // and the receiver simply gets subscribed and reads packets once it's ready.
    //
    // Backup: The link stays idle until it's activated, and the activation can only happen
    // at the moment when the currently active link is "suspected of being likely broken"
    // (the current active link fails to receive ACK in a time when two ACKs should already
    // be received). After a while when the current active link is confirmed broken, it turns
    // into broken state.

    static const char* StateStr(GroupState);

    static int32_t s_tokenGen;
    static int32_t genToken() { ++s_tokenGen; if (s_tokenGen < 0) s_tokenGen = 0; return s_tokenGen;}

    struct SocketData
    {
        SRTSOCKET      id;
        CUDTSocket*    ps;
        int            token;
        SRT_SOCKSTATUS laststatus;
        GroupState     sndstate;
        GroupState     rcvstate;
        int            sndresult;
        int            rcvresult;
        sockaddr_any   agent;
        sockaddr_any   peer;
        bool           ready_read;
        bool           ready_write;
        bool           ready_error;

        // Configuration
        uint16_t weight;
    };

    struct ConfigItem
    {
        SRT_SOCKOPT                so;
        std::vector<unsigned char> value;

        template <class T>
        bool get(T& refr)
        {
            if (sizeof(T) > value.size())
                return false;
            refr = *(T*)&value[0];
            return true;
        }

        ConfigItem(SRT_SOCKOPT o, const void* val, int size)
            : so(o)
        {
            value.resize(size);
            unsigned char* begin = (unsigned char*)val;
            std::copy(begin, begin + size, value.begin());
        }

        struct OfType
        {
            SRT_SOCKOPT so;
            OfType(SRT_SOCKOPT soso)
                : so(soso)
            {
            }
            bool operator()(ConfigItem& ci) { return ci.so == so; }
        };
    };

    typedef std::list<SocketData> group_t;
    typedef group_t::iterator     gli_t;

    struct Sendstate
    {
        gli_t d;
        int   stat;
        int   code;
    };

    CUDTGroup(SRT_GROUP_TYPE);
    ~CUDTGroup();

    static SocketData prepareData(CUDTSocket* s);

    gli_t add(SocketData data);

    struct HaveID
    {
        SRTSOCKET id;
        HaveID(SRTSOCKET sid)
            : id(sid)
        {
        }
        bool operator()(const SocketData& s) { return s.id == id; }
    };

    gli_t find(SRTSOCKET id)
    {
        srt::sync::ScopedLock g(m_GroupLock);
        gli_t                 f = std::find_if(m_Group.begin(), m_Group.end(), HaveID(id));
        if (f == m_Group.end())
        {
            return gli_NULL();
        }
        return f;
    }

    // NEED LOCKING
    gli_t begin() { return m_Group.begin(); }
    gli_t end() { return m_Group.end(); }

    // REMEMBER: the group spec should be taken from the socket
    // (set m_IncludedGroup to NULL and m_IncludedIter to grp->gli_NULL())
    // PRIOR TO calling this function.
    bool remove(SRTSOCKET id)
    {
        bool                  s = false;
        srt::sync::ScopedLock g(m_GroupLock);
        gli_t                 f = std::find_if(m_Group.begin(), m_Group.end(), HaveID(id));
        if (f != m_Group.end())
        {
            m_Group.erase(f);

            // Reset sequence numbers on a dead group so that they are
            // initialized anew with the new alive connection within
            // the group.
            // XXX The problem is that this should be done after the
            // socket is considered DISCONNECTED, not when it's being
            // closed. After being disconnected, the sequence numbers
            // are no longer valid, and will be reinitialized when the
            // socket is connected again. This may stay as is for now
            // as in SRT it's not predicted to do anything with the socket
            // that was disconnected other than immediately closing it.
            if (m_Group.empty())
            {
                // When the group is empty, there's no danger that this
                // number will collide with any ISN provided by a socket.
                // Also since now every socket will derive this ISN.
                m_iLastSchedSeqNo = generateISN();
                resetInitialRxSequence();
            }
            s = true;
        }

        if (m_Group.empty())
        {
            m_bOpened    = false;
            m_bConnected = false;
        }

        // XXX BUGFIX
        m_Positions.erase(id);

        return s;
    }

    bool empty()
    {
        srt::sync::ScopedLock g(m_GroupLock);
        return m_Group.empty();
    }

    void setFreshConnected(CUDTSocket* sock, int& w_token);

    static gli_t gli_NULL() { return GroupContainer::null(); }

    int            send(const char* buf, int len, SRT_MSGCTRL& w_mc);
    int            sendBroadcast(const char* buf, int len, SRT_MSGCTRL& w_mc);
    int            sendBackup(const char* buf, int len, SRT_MSGCTRL& w_mc);
    static int32_t generateISN();

private:
    // For Backup, sending all previous packet
    int sendBackupRexmit(CUDT& core, SRT_MSGCTRL& w_mc);

    // Support functions for sendBackup and sendBroadcast
    bool send_CheckIdle(const gli_t d, std::vector<gli_t>& w_wipeme, std::vector<gli_t>& w_pending);
    void sendBackup_CheckIdleTime(gli_t w_d);
    bool sendBackup_CheckRunningStability(const gli_t d, const time_point currtime);
    bool sendBackup_CheckSendStatus(const gli_t         d,
                                    const time_point&   currtime,
                                    const int           stat,
                                    const int           erc,
                                    const int32_t       lastseq,
                                    const int32_t       pktseq,
                                    CUDT&               w_u,
                                    int32_t&            w_curseq,
                                    std::vector<gli_t>& w_parallel,
                                    int&                w_final_stat,
                                    std::set<uint16_t>& w_sendable_pri,
                                    size_t&             w_nsuccessful,
                                    bool&               w_is_unstable);
    void sendBackup_Buffering(const char* buf, const int len, int32_t& curseq, SRT_MSGCTRL& w_mc);
    size_t sendBackup_CheckNeedActivate(const std::vector<gli_t>& idlers,
                                      const char*               buf,
                                      const int                 len,
                                      bool&                     w_none_succeeded,
                                      SRT_MSGCTRL&              w_mc,
                                      int32_t&                  w_curseq,
                                      int32_t&                  w_final_stat,
                                      CUDTException&            w_cx,
                                      std::vector<Sendstate>&   w_sendstates,
                                      std::vector<gli_t>&       w_parallel,
                                      std::vector<gli_t>&       w_wipeme,
                                      const std::string&        activate_reason);
    void send_CheckPendingSockets(const std::vector<gli_t>& pending, std::vector<gli_t>& w_wipeme);
    void send_CloseBrokenSockets(std::vector<gli_t>& w_wipeme);
    void sendBackup_CheckParallelLinks(const std::vector<gli_t>& unstable,
                                       std::vector<gli_t>&       w_parallel,
                                       int&                      w_final_stat,
                                       bool&                     w_none_succeeded,
                                       SRT_MSGCTRL&              w_mc,
                                       CUDTException&            w_cx);

public:
    int recv(char* buf, int len, SRT_MSGCTRL& w_mc);

    void close();

    void setOpt(SRT_SOCKOPT optname, const void* optval, int optlen);
    void getOpt(SRT_SOCKOPT optName, void* optval, int& w_optlen);
    void deriveSettings(CUDT* source);
    bool applyFlags(uint32_t flags, HandshakeSide);

    SRT_SOCKSTATUS getStatus();

    void debugMasterData(SRTSOCKET slave);

    bool isGroupReceiver()
    {
        // XXX add here also other group types, which
        // predict group receiving.
        return m_type == SRT_GTYPE_BROADCAST;
    }

    srt::sync::Mutex* exp_groupLock() { return &m_GroupLock; }
    void              addEPoll(int eid);
    void              removeEPollEvents(const int eid);
    void              removeEPollID(const int eid);
    void              updateReadState(SRTSOCKET sock, int32_t sequence);
    void              updateWriteState();
    int               updateFailedLink(SRTSOCKET sock);
    void              activateUpdateEvent();

    /// Update the in-group array of packet providers per sequence number.
    /// Also basing on the information already provided by possibly other sockets,
    /// report the real status of packet loss, including packets maybe lost
    /// by the caller provider, but already received from elsewhere. Note that
    /// these packets are not ready for extraction until ACK-ed.
    ///
    /// @param exp_sequence The previously received sequence at this socket
    /// @param sequence The sequence of this packet
    /// @param provider The core of the socket for which the packet was dispatched
    /// @param time TSBPD time of this packet
    /// @return The bitmap that marks by 'false' packets lost since next to exp_sequence
    std::vector<bool> providePacket(int32_t exp_sequence, int32_t sequence, CUDT* provider, uint64_t time);

    /// This is called from the ACK action by particular socket, which
    /// actually signs off the packet for extraction.
    ///
    /// @param core The socket core for which the ACK was sent
    /// @param ack The past-the-last-received ACK sequence number
    void readyPackets(CUDT* core, int32_t ack);

    void syncWithSocket(const CUDT& core, const HandshakeSide side);
    int  getGroupData(SRT_SOCKGROUPDATA* pdata, size_t* psize);
    int  getGroupDataIn(SRT_SOCKGROUPDATA* pdata, size_t* psize);
    int  configure(const char* str);

    /// Predicted to be called from the reading function to fill
    /// the group data array as requested.
    void fillGroupData(SRT_MSGCTRL&       w_out, //< MSGCTRL to be written
                       const SRT_MSGCTRL& in     //< MSGCTRL read from the data-providing socket
    );

    void copyGroupData(const CUDTGroup::SocketData& source, SRT_SOCKGROUPDATA& w_target);

#if ENABLE_HEAVY_LOGGING
    void debugGroup();
#else
    void debugGroup() {}
#endif

    void ackMessage(int32_t msgno);
    void handleKeepalive(gli_t);
    void internalKeepalive(gli_t);

private:
    // Check if there's at least one connected socket.
    // If so, grab the status of all member sockets.
    void getGroupCount(size_t& w_size, bool& w_still_alive);

    class CUDTUnited* m_pGlobal;
    srt::sync::Mutex  m_GroupLock;

    SRTSOCKET m_GroupID;
    SRTSOCKET m_PeerGroupID;
    struct GroupContainer
    {
        std::list<SocketData>        m_List;
        static std::list<SocketData> s_NoList; // This is to have a predictable "null iterator".

        /// This field is used only by some types of groups that need
        /// to keep track as to which link was lately used. Note that
        /// by removal of a node from the m_List container, this link
        /// must be appropriately reset.
        gli_t m_LastActiveLink;

        GroupContainer()
            : m_LastActiveLink(s_NoList.begin())
        {
        }

        // Property<gli_t> active = { m_LastActiveLink; }
        SRTU_PROPERTY_RW(gli_t, active, m_LastActiveLink);

        gli_t        begin() { return m_List.begin(); }
        gli_t        end() { return m_List.end(); }
        static gli_t null() { return s_NoList.begin(); }
        bool         empty() { return m_List.empty(); }
        void         push_back(const SocketData& data) { m_List.push_back(data); }
        void         clear()
        {
            m_LastActiveLink = null();
            m_List.clear();
        }
        size_t size() { return m_List.size(); }

        void erase(gli_t it);
    };
    GroupContainer m_Group;
    bool           m_selfManaged;
    bool           m_bSyncOnMsgNo;
    SRT_GROUP_TYPE m_type;
    CUDTSocket*    m_listener; // A "group" can only have one listener.
    CallbackHolder<srt_connect_callback_fn> m_cbConnectHook;
    void installConnectHook(srt_connect_callback_fn* hook, void* opaq)
    {
        m_cbConnectHook.set(opaq, hook);
    }

public:
    struct BufferedMessageStorage
    {
        size_t             blocksize;
        size_t             maxstorage;
        std::vector<char*> storage;

        BufferedMessageStorage(size_t blk, size_t max = 0)
            : blocksize(blk)
            , maxstorage(max)
            , storage()
        {
        }

        char* get()
        {
            if (storage.empty())
                return new char[blocksize];

            // Get the element from the end
            char* block = storage.back();
            storage.pop_back();
            return block;
        }

        void put(char* block)
        {
            if (storage.size() >= maxstorage)
            {
                // Simply delete
                delete[] block;
                return;
            }

            // Put the block into the spare buffer
            storage.push_back(block);
        }

        ~BufferedMessageStorage()
        {
            for (size_t i = 0; i < storage.size(); ++i)
                delete[] storage[i];
        }
    };

    struct BufferedMessage
    {
        static BufferedMessageStorage storage;

        SRT_MSGCTRL mc;
        char*       data;
        size_t      size;

        BufferedMessage()
            : data()
            , size()
        {
        }
        ~BufferedMessage()
        {
            if (data)
                storage.put(data);
        }

        // NOTE: size 's' must be checked against SRT_LIVE_MAX_PLSIZE
        // before calling
        void copy(const char* buf, size_t s)
        {
            size = s;
            data = storage.get();
            memcpy(data, buf, s);
        }

        BufferedMessage(const BufferedMessage& foreign SRT_ATR_UNUSED)
            : data()
            , size()
        {
            // This is only to copy empty container.
            // Any other use should not be done.
            //#if ENABLE_DEBUG
            //            if (foreign.data)
            //                abort();
            //#endif
        }

    private:
        void swap_with(BufferedMessage& b)
        {
            std::swap(this->mc, b.mc);
            std::swap(this->data, b.data);
            std::swap(this->size, b.size);
        }
    };

    typedef std::deque<BufferedMessage> senderBuffer_t;
    // typedef StaticBuffer<BufferedMessage, 1000> senderBuffer_t;

private:
    // Fields required for SRT_GTYPE_BACKUP groups.
    senderBuffer_t   m_SenderBuffer;
    int32_t          m_iSndOldestMsgNo; // oldest position in the sender buffer
    volatile int32_t m_iSndAckedMsgNo;
    uint32_t         m_uOPT_StabilityTimeout;

    // THIS function must be called only in a function for a group type
    // that does use sender buffer.
    int32_t addMessageToBuffer(const char* buf, size_t len, SRT_MSGCTRL& w_mc);

    std::set<int>      m_sPollID; // set of epoll ID to trigger
    int                m_iMaxPayloadSize;
    int                m_iAvgPayloadSize;
    bool               m_bSynRecving;
    bool               m_bSynSending;
    bool               m_bTsbPd;
    bool               m_bTLPktDrop;
    int64_t            m_iTsbPdDelay_us;
    int                m_RcvEID;
    struct CEPollDesc* m_RcvEpolld;
    int                m_SndEID;
    struct CEPollDesc* m_SndEpolld;

    int m_iSndTimeOut; // sending timeout in milliseconds
    int m_iRcvTimeOut; // receiving timeout in milliseconds

    // Start times for TsbPd. These times shall be synchronized
    // between all sockets in the group. The first connected one
    // defines it, others shall derive it. The value 0 decides if
    // this has been already set.
    time_point m_tsStartTime;
    time_point m_tsRcvPeerStartTime;

    struct ReadPos
    {
        std::vector<char> packet;
        SRT_MSGCTRL       mctrl;
        ReadPos(int32_t s)
            : mctrl(srt_msgctrl_default)
        {
            mctrl.pktseq = s;
        }
    };
    std::map<SRTSOCKET, ReadPos> m_Positions;

    ReadPos* checkPacketAhead();

    // This is the sequence number of a packet that has been previously
    // delivered. Initially it should be set to SRT_SEQNO_NONE so that the sequence read
    // from the first delivering socket will be taken as a good deal.
    volatile int32_t m_RcvBaseSeqNo;

    bool m_bOpened;    // Set to true when at least one link is at least pending
    bool m_bConnected; // Set to true on first link confirmed connected
    bool m_bClosing;

    // There's no simple way of transforming config
    // items that are predicted to be used on socket.
    // Use some options for yourself, store the others
    // for setting later on a socket.
    std::vector<ConfigItem> m_config;

    // Signal for the blocking user thread that the packet
    // is ready to deliver.
    srt::sync::Condition m_RcvDataCond;
    srt::sync::Mutex     m_RcvDataLock;
    volatile int32_t     m_iLastSchedSeqNo; // represetnts the value of CUDT::m_iSndNextSeqNo for each running socket
    volatile int32_t     m_iLastSchedMsgNo;
    // Statistics

    struct Stats
    {
        // Stats state
        time_point tsActivateTime;   // Time when this group sent or received the first data packet
        time_point tsLastSampleTime; // Time reset when clearing stats

        MetricUsage<PacketMetric> sent; // number of packets sent from the application
        MetricUsage<PacketMetric> recv; // number of packets delivered from the group to the application
        MetricUsage<PacketMetric>
                                  recvDrop; // number of packets dropped by the group receiver (not received from any member)
        MetricUsage<PacketMetric> recvDiscard; // number of packets discarded as already delivered

        void init()
        {
            tsActivateTime = srt::sync::steady_clock::time_point();
            sent.Init();
            recv.Init();
            recvDrop.Init();
            recvDiscard.Init();

            reset();
        }

        void reset()
        {
            sent.Clear();
            recv.Clear();
            recvDrop.Clear();
            recvDiscard.Clear();

            tsLastSampleTime = srt::sync::steady_clock::now();
        }
    } m_stats;

    void updateAvgPayloadSize(int size)
    {
        if (m_iAvgPayloadSize == -1)
            m_iAvgPayloadSize = size;
        else
            m_iAvgPayloadSize = avg_iir<4>(m_iAvgPayloadSize, size);
    }

    int avgRcvPacketSize()
    {
        // In case when no packet has been received yet, but already notified
        // a dropped packet, its size will be SRT_LIVE_DEF_PLSIZE. It will be
        // the value most matching in the typical uses, although no matter what
        // value would be used here, each one would be wrong from some points
        // of view. This one is simply the best choice for typical uses of groups
        // provided that they are to be ued only for live mode.
        return m_iAvgPayloadSize == -1 ? SRT_LIVE_DEF_PLSIZE : m_iAvgPayloadSize;
    }

public:
    void bstatsSocket(CBytePerfMon* perf, bool clear);

    // Required after the call on newGroup on the listener side.
    // On the listener side the group is lazily created just before
    // accepting a new socket and therefore always open.
    void setOpen() { m_bOpened = true; }

    std::string CONID() const
    {
#if ENABLE_LOGGING
        std::ostringstream os;
        os << "@" << m_GroupID << ":";
        return os.str();
#else
        return "";
#endif
    }

    void resetInitialRxSequence()
    {
        // The app-reader doesn't care about the real sequence number.
        // The first provided one will be taken as a good deal; even if
        // this is going to be past the ISN, at worst it will be caused
        // by TLPKTDROP.
        m_RcvBaseSeqNo = SRT_SEQNO_NONE;
    }

    bool applyGroupTime(time_point& w_start_time, time_point& w_peer_start_time)
    {
        using srt::sync::is_zero;
        using srt_logging::gmlog;

        if (is_zero(m_tsStartTime))
        {
            // The first socket, defines the group time for the whole group.
            m_tsStartTime        = w_start_time;
            m_tsRcvPeerStartTime = w_peer_start_time;
            return true;
        }

        // Sanity check. This should never happen, fix the bug if found!
        if (is_zero(m_tsRcvPeerStartTime))
        {
            LOGC(gmlog.Error, log << "IPE: only StartTime is set, RcvPeerStartTime still 0!");
            // Kinda fallback, but that's not too safe.
            m_tsRcvPeerStartTime = w_peer_start_time;
        }

        // The redundant connection, derive the times
        w_start_time      = m_tsStartTime;
        w_peer_start_time = m_tsRcvPeerStartTime;

        return false;
    }

    // Live state synchronization
    bool getBufferTimeBase(CUDT* forthesakeof, time_point& w_tb, bool& w_wp, duration& w_dr);
    bool applyGroupSequences(SRTSOCKET, int32_t& w_snd_isn, int32_t& w_rcv_isn);
    void synchronizeDrift(CUDT* cu, duration udrift, time_point newtimebase);

    void updateLatestRcv(gli_t);

    // Property accessors
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, SRTSOCKET, id, m_GroupID);
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, SRTSOCKET, peerid, m_PeerGroupID);
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, bool, managed, m_selfManaged);
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, SRT_GROUP_TYPE, type, m_type);
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, int32_t, currentSchedSequence, m_iLastSchedSeqNo);
    SRTU_PROPERTY_RRW(std::set<int>&, epollset, m_sPollID);
    SRTU_PROPERTY_RW_CHAIN(CUDTGroup, int64_t, latency, m_iTsbPdDelay_us);
    SRTU_PROPERTY_RO(bool, synconmsgno, m_bSyncOnMsgNo);
};

#endif // INC_SRT_GROUP_H

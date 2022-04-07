/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2018 Haivision Systems Inc.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 */

/*****************************************************************************
Copyright (c) 2001 - 2011, The Board of Trustees of the University of Illinois.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions
  and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the University of Illinois
  nor the names of its contributors may be used to
  endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

/*****************************************************************************
written by
   Yunhong Gu, last updated 02/28/2012
modified by
   Haivision Systems Inc.
*****************************************************************************/


#ifndef INC_SRT_CORE_H
#define INC_SRT_CORE_H

#include <deque>
#include <sstream>

#include "srt.h"
#include "common.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "packet.h"
#include "channel.h"
#include "cache.h"
#include "queue.h"
#include "handshake.h"
#include "congctl.h"
#include "packetfilter.h"
#include "utilities.h"
#include "logger_defs.h"

#include <haicrypt.h>


// XXX Utility function - to be moved to utilities.h?
template <class T>
inline T CountIIR(T base, T newval, double factor)
{
    if ( base == 0.0 )
        return newval;

    T diff = newval - base;
    return base+T(diff*factor);
}

// XXX Probably a better rework for that can be done - this can be
// turned into a serializable structure, just like it's for CHandShake.
enum AckDataItem
{
    ACKD_RCVLASTACK = 0,
    ACKD_RTT = 1,
    ACKD_RTTVAR = 2,
    ACKD_BUFFERLEFT = 3,
    ACKD_TOTAL_SIZE_SMALL = 4,

    // Extra fields existing in UDT (not always sent)

    ACKD_RCVSPEED = 4,   // length would be 16
    ACKD_BANDWIDTH = 5,
    ACKD_TOTAL_SIZE_UDTBASE = 6, // length = 24
    // Extra stats for SRT

    ACKD_RCVRATE = 6,
    ACKD_TOTAL_SIZE_VER101 = 7, // length = 28
    ACKD_XMRATE = 7, // XXX This is a weird compat stuff. Version 1.1.3 defines it as ACKD_BANDWIDTH*m_iMaxSRTPayloadSize when set. Never got.
                     // XXX NOTE: field number 7 may be used for something in future, need to confirm destruction of all !compat 1.0.2 version

    ACKD_TOTAL_SIZE_VER102 = 8, // 32
// FEATURE BLOCKED. Probably not to be restored.
//  ACKD_ACKBITMAP = 8,
    ACKD_TOTAL_SIZE = ACKD_TOTAL_SIZE_VER102 // length = 32 (or more)
};
const size_t ACKD_FIELD_SIZE = sizeof(int32_t);

static const size_t SRT_SOCKOPT_NPOST = 12;
extern const SRT_SOCKOPT srt_post_opt_list [];

enum GroupDataItem
{
    GRPD_GROUPID,
    GRPD_GROUPDATA,

    GRPD_E_SIZE
};

const size_t GRPD_MIN_SIZE = 2; // ID and GROUPTYPE as backward compat

const size_t GRPD_FIELD_SIZE = sizeof(int32_t);

// For HSv4 legacy handshake
#define SRT_MAX_HSRETRY     10          /* Maximum SRT handshake retry */

enum SeqPairItems
{
    SEQ_BEGIN = 0, SEQ_END = 1, SEQ_SIZE = 2
};

#if ENABLE_EXPERIMENTAL_BONDING

struct SRT_SocketOptionObject
{
    struct SingleOption
    {
        uint16_t option;
        uint16_t length;
        unsigned char storage[1]; // NOTE: Variable length object!
    };

    std::vector<SingleOption*> options;

    SRT_SocketOptionObject() {}

    ~SRT_SocketOptionObject()
    {
        for (size_t i = 0; i < options.size(); ++i)
        {
            // Convert back
            unsigned char* mem = reinterpret_cast<unsigned char*>(options[i]);
            delete [] mem;
        }
    }

    bool add(SRT_SOCKOPT optname, const void* optval, size_t optlen);
};

class CUDTGroup;
#endif

template <typename T>
inline T cast_optval(const void* optval)
{
    return *reinterpret_cast<const T*>(optval);
}

template <typename T>
inline T cast_optval(const void* optval, int optlen)
{
    if (optlen > 0 && optlen != sizeof(T))
        throw CUDTException(MJ_NOTSUP, MN_INVAL, 0);

    return cast_optval<T>(optval);
}

// This function is to make it possible for both C and C++
// API to accept both bool and int types for boolean options.
// (it's not that C couldn't use <stdbool.h>, it's that people
// often forget to use correct type).
template <>
inline bool cast_optval(const void* optval, int optlen)
{
    if (optlen == sizeof(bool))
    {
        return *reinterpret_cast<const bool*>(optval);
    }

    if (optlen == sizeof(int))
    {
        // 0!= is a windows warning-killer int-to-bool conversion
        return 0 != *reinterpret_cast<const int*>(optval);
    }
    return false;
}

// Extended SRT Congestion control class - only an incomplete definition required
class CCryptoControl;
class CUDTUnited;
class CUDTSocket;

// XXX REFACTOR: The 'CUDT' class is to be merged with 'CUDTSocket'.
// There's no reason for separating them, there's no case of having them
// anyhow managed separately. After this is done, with a small help with
// separating the internal abnormal path management (exceptions) from the
// API (return values), through CUDTUnited, this class may become in future
// an officially exposed C++ API.
class CUDT
{
    friend class CUDTSocket;
    friend class CUDTUnited;
    friend class CCC;
    friend struct CUDTComp;
    friend class CCache<CInfoBlock>;
    friend class CRendezvousQueue;
    friend class CSndQueue;
    friend class CRcvQueue;
    friend class CSndUList;
    friend class CRcvUList;
    friend class PacketFilter;
    friend class CUDTGroup;
    friend struct FByOldestActive; // this functional will use private fields

    typedef srt::sync::steady_clock::time_point time_point;
    typedef srt::sync::steady_clock::duration duration;

private: // constructor and desctructor
    void construct();
    void clearData();
    CUDT(CUDTSocket* parent);
    CUDT(CUDTSocket* parent, const CUDT& ancestor);
    const CUDT& operator=(const CUDT&) {return *this;} // = delete ?
    ~CUDT();

public: //API
    static int startup();
    static int cleanup();
    static SRTSOCKET socket();
#if ENABLE_EXPERIMENTAL_BONDING
    static SRTSOCKET createGroup(SRT_GROUP_TYPE);
    static int addSocketToGroup(SRTSOCKET socket, SRTSOCKET group);
    static int removeSocketFromGroup(SRTSOCKET socket);
    static SRTSOCKET getGroupOfSocket(SRTSOCKET socket);
    static int getGroupData(SRTSOCKET groupid, SRT_SOCKGROUPDATA* pdata, size_t* psize);
    static int configureGroup(SRTSOCKET groupid, const char* str);
    static bool isgroup(SRTSOCKET sock) { return (sock & SRTGROUP_MASK) != 0; }
#endif
    static int bind(SRTSOCKET u, const sockaddr* name, int namelen);
    static int bind(SRTSOCKET u, UDPSOCKET udpsock);
    static int listen(SRTSOCKET u, int backlog);
    static SRTSOCKET accept(SRTSOCKET u, sockaddr* addr, int* addrlen);
    static SRTSOCKET accept_bond(const SRTSOCKET listeners [], int lsize, int64_t msTimeOut);
    static int connect(SRTSOCKET u, const sockaddr* name, int namelen, int32_t forced_isn);
    static int connect(SRTSOCKET u, const sockaddr* name, const sockaddr* tname, int namelen);
#if ENABLE_EXPERIMENTAL_BONDING
    static int connectLinks(SRTSOCKET grp, SRT_SOCKGROUPCONFIG links [], int arraysize);
#endif
    static int close(SRTSOCKET u);
    static int getpeername(SRTSOCKET u, sockaddr* name, int* namelen);
    static int getsockname(SRTSOCKET u, sockaddr* name, int* namelen);
    static int getsockopt(SRTSOCKET u, int level, SRT_SOCKOPT optname, void* optval, int* optlen);
    static int setsockopt(SRTSOCKET u, int level, SRT_SOCKOPT optname, const void* optval, int optlen);
    static int send(SRTSOCKET u, const char* buf, int len, int flags);
    static int recv(SRTSOCKET u, char* buf, int len, int flags);
    static int sendmsg(SRTSOCKET u, const char* buf, int len, int ttl = SRT_MSGTTL_INF, bool inorder = false, int64_t srctime = 0);
    static int recvmsg(SRTSOCKET u, char* buf, int len, int64_t& srctime);
    static int sendmsg2(SRTSOCKET u, const char* buf, int len, SRT_MSGCTRL& mctrl);
    static int recvmsg2(SRTSOCKET u, char* buf, int len, SRT_MSGCTRL& w_mctrl);
    static int64_t sendfile(SRTSOCKET u, std::fstream& ifs, int64_t& offset, int64_t size, int block = SRT_DEFAULT_SENDFILE_BLOCK);
    static int64_t recvfile(SRTSOCKET u, std::fstream& ofs, int64_t& offset, int64_t size, int block = SRT_DEFAULT_RECVFILE_BLOCK);
    static int select(int nfds, UDT::UDSET* readfds, UDT::UDSET* writefds, UDT::UDSET* exceptfds, const timeval* timeout);
    static int selectEx(const std::vector<SRTSOCKET>& fds, std::vector<SRTSOCKET>* readfds, std::vector<SRTSOCKET>* writefds, std::vector<SRTSOCKET>* exceptfds, int64_t msTimeOut);
    static int epoll_create();
    static int epoll_clear_usocks(int eid);
    static int epoll_add_usock(const int eid, const SRTSOCKET u, const int* events = NULL);
    static int epoll_add_ssock(const int eid, const SYSSOCKET s, const int* events = NULL);
    static int epoll_remove_usock(const int eid, const SRTSOCKET u);
    static int epoll_remove_ssock(const int eid, const SYSSOCKET s);
    static int epoll_update_usock(const int eid, const SRTSOCKET u, const int* events = NULL);
    static int epoll_update_ssock(const int eid, const SYSSOCKET s, const int* events = NULL);
    static int epoll_wait(const int eid, std::set<SRTSOCKET>* readfds, std::set<SRTSOCKET>* writefds,
            int64_t msTimeOut, std::set<SYSSOCKET>* lrfds = NULL, std::set<SYSSOCKET>* wrfds = NULL);
    static int epoll_uwait(const int eid, SRT_EPOLL_EVENT* fdsSet, int fdsSize, int64_t msTimeOut);
    static int32_t epoll_set(const int eid, int32_t flags);
    static int epoll_release(const int eid);
    static CUDTException& getlasterror();
    static int bstats(SRTSOCKET u, CBytePerfMon* perf, bool clear = true, bool instantaneous = false);
#if ENABLE_EXPERIMENTAL_BONDING
    static int groupsockbstats(SRTSOCKET u, CBytePerfMon* perf, bool clear = true);
#endif
    static SRT_SOCKSTATUS getsockstate(SRTSOCKET u);
    static bool setstreamid(SRTSOCKET u, const std::string& sid);
    static std::string getstreamid(SRTSOCKET u);
    static int getsndbuffer(SRTSOCKET u, size_t* blocks, size_t* bytes);
    static int rejectReason(SRTSOCKET s);
    static int rejectReason(SRTSOCKET s, int value);
    static int64_t socketStartTime(SRTSOCKET s);

public: // internal API
    // This is public so that it can be used directly in API implementation functions.
    struct APIError
    {
        APIError(const CUDTException&);
        APIError(CodeMajor, CodeMinor, int = 0);

        operator int() const
        {
            return SRT_ERROR;
        }
    };

    static const SRTSOCKET INVALID_SOCK = -1;         // invalid socket descriptor
    static const int ERROR = -1;                      // socket api error returned value

    static const int HS_VERSION_UDT4 = 4;
    static const int HS_VERSION_SRT1 = 5;

    // Parameters
    //
    // Note: use notation with X*1000*1000* ... instead of million zeros in a row.
    // In C++17 there is a possible notation of 5'000'000 for convenience, but that's
    // something only for a far future.
    static const int COMM_RESPONSE_TIMEOUT_MS = 5*1000; // 5 seconds
    static const int COMM_RESPONSE_MAX_EXP = 16;
    static const int SRT_TLPKTDROP_MINTHRESHOLD_MS = 1000;
    static const uint64_t COMM_KEEPALIVE_PERIOD_US = 1*1000*1000;
    static const int32_t COMM_SYN_INTERVAL_US = 10*1000;
    static const uint32_t COMM_DEF_STABILITY_TIMEOUT_US = 80*1000;
    static const int COMM_CLOSE_BROKEN_LISTENER_TIMEOUT_MS = 3000;
    static const uint16_t MAX_WEIGHT = 32767;

    static const int
        DEF_MSS = 1500,
        DEF_FLIGHT_SIZE = 25600,
        DEF_BUFFER_SIZE = 8192, //Rcv buffer MUST NOT be bigger than Flight Flag size
        DEF_LINGER_S = 3*60,  // 3 minutes
        DEF_UDP_BUFFER_SIZE = 65536,
        DEF_CONNTIMEO_S = 3; // 3 seconds


    int handshakeVersion()
    {
        return m_ConnRes.m_iVersion;
    }

    std::string CONID() const
    {
#if ENABLE_LOGGING
        std::ostringstream os;
        os << "@" << m_SocketID << ":";
        return os.str();
#else
        return "";
#endif
    }

    SRTSOCKET socketID() const { return m_SocketID; }

    static CUDT* getUDTHandle(SRTSOCKET u);
    static std::vector<SRTSOCKET> existingSockets();

    void addressAndSend(CPacket& pkt);
    void sendSrtMsg(int cmd, uint32_t *srtdata_in = NULL, int srtlen_in = 0);

    bool isOPT_TsbPd() const { return m_bOPT_TsbPd; }
    int RTT() const { return m_iRTT; }
    int32_t sndSeqNo() const { return m_iSndCurrSeqNo; }
    int32_t schedSeqNo() const { return m_iSndNextSeqNo; }
    bool overrideSndSeqNo(int32_t seq);

    int32_t rcvSeqNo() const { return m_iRcvCurrSeqNo; }
    int flowWindowSize() const { return m_iFlowWindowSize; }
    int32_t deliveryRate() const { return m_iDeliveryRate; }
    int bandwidth() const { return m_iBandwidth; }
    int64_t maxBandwidth() const { return m_llMaxBW; }
    int MSS() const { return m_iMSS; }

    uint32_t latency_us() const {return m_iTsbPdDelay_ms*1000; }
    size_t maxPayloadSize() const { return m_iMaxSRTPayloadSize; }
    size_t OPT_PayloadSize() const { return m_zOPT_ExpPayloadSize; }
    int sndLossLength() { return m_pSndLossList->getLossLength(); }
    int32_t ISN() const { return m_iISN; }
    int32_t peerISN() const { return m_iPeerISN; }
    duration minNAKInterval() const { return m_tdMinNakInterval; }
    sockaddr_any peerAddr() const { return m_PeerAddr; }

    int32_t getFlightSpan() const
    {
        // This is a number of unacknowledged packets at this moment
        // Note that normally m_iSndLastAck should be PAST m_iSndCurrSeqNo,
        // however in a case when the sending stopped and all packets were
        // ACKed, the m_iSndLastAck is one sequence ahead of m_iSndCurrSeqNo.
        // Therefore we increase m_iSndCurrSeqNo by 1 forward and then
        // get the distance towards the last ACK. This way this value may
        // be only positive or 0.

        return CSeqNo::seqlen(m_iSndLastAck, CSeqNo::incseq(m_iSndCurrSeqNo));
    }

    int minSndSize(int len = 0) const
    {
        if (len == 0) // wierd, can't use non-static data member as default argument!
            len = m_iMaxSRTPayloadSize;
        return m_bMessageAPI ? (len+m_iMaxSRTPayloadSize-1)/m_iMaxSRTPayloadSize : 1;
    }

    int32_t makeTS(const time_point& from_time) const
    {
        // NOTE:
        // - This calculates first the time difference towards start time.
        // - This difference value is also CUT OFF THE SEGMENT information
        //   (a multiple of MAX_TIMESTAMP+1)
        // So, this can be simply defined as: TS = (RTS - STS) % (MAX_TIMESTAMP+1)
        // XXX Would be nice to check if local_time > m_tsStartTime,
        // otherwise it may go unnoticed with clock skew.
        return srt::sync::count_microseconds(from_time - m_stats.tsStartTime);
    }

    void setPacketTS(CPacket& p, const time_point& local_time)
    {
        p.m_iTimeStamp = makeTS(local_time);
    }

    // Utility used for closing a listening socket
    // immediately to free the socket
    void notListening()
    {
        srt::sync::ScopedLock cg(m_ConnectionLock);
        m_bListening = false;
        m_pRcvQueue->removeListener(this);
    }

    static int32_t generateISN()
    {
        using namespace srt::sync;
        // Random Initial Sequence Number (normal mode)
        srand(count_microseconds(steady_clock::now().time_since_epoch()));
        return (int32_t)(CSeqNo::m_iMaxSeqNo * (double(rand()) / RAND_MAX));
    }

    // XXX See CUDT::tsbpd() to see how to implement it. This should
    // do the same as TLPKTDROP feature when skipping packets that are agreed
    // to be lost. Note that this is predicted to be called with TSBPD off.
    // This is to be exposed for the application so that it can require this
    // sequence to be skipped, if that packet has been otherwise arrived through
    // a different channel.
    void skipIncoming(int32_t seq);

    // For SRT_tsbpdLoop
    CUDTUnited* uglobal() { return &s_UDTUnited; } // needed by tsbpdLoop
    std::set<int>& pollset() { return m_sPollID; }

    SRTU_PROPERTY_RO(SRTSOCKET, id, m_SocketID);
    SRTU_PROPERTY_RO(bool, isClosing, m_bClosing);
    SRTU_PROPERTY_RO(CRcvBuffer*, rcvBuffer, m_pRcvBuffer);
    SRTU_PROPERTY_RO(bool, isTLPktDrop, m_bTLPktDrop);
    SRTU_PROPERTY_RO(bool, isSynReceiving, m_bSynRecving);
    SRTU_PROPERTY_RR(srt::sync::Condition*, recvDataCond, &m_RecvDataCond);
    SRTU_PROPERTY_RR(srt::sync::Condition*, recvTsbPdCond, &m_RcvTsbPdCond);

    void ConnectSignal(ETransmissionEvent tev, EventSlot sl);
    void DisconnectSignal(ETransmissionEvent tev);

    // This is in public section so prospective overriding it can be
    // done by directly assigning to a field.

    typedef std::vector< std::pair<int32_t, int32_t> > loss_seqs_t;
    typedef loss_seqs_t packetArrival_cb(void*, CPacket&);
    CallbackHolder<packetArrival_cb> m_cbPacketArrival;

private:
    /// initialize a UDT entity and bind to a local address.

    void open();

    /// Start listening to any connection request.

    void setListenState();

    /// Connect to a UDT entity listening at address "peer".
    /// @param peer [in] The address of the listening UDT entity.

    void startConnect(const sockaddr_any& peer, int32_t forced_isn);

    /// Process the response handshake packet. Failure reasons can be:
    /// * Socket is not in connecting state
    /// * Response @a pkt is not a handshake control message
    /// * Rendezvous socket has once processed a regular handshake
    /// @param pkt [in] handshake packet.
    /// @retval 0 Connection successful
    /// @retval 1 Connection in progress (m_ConnReq turned into RESPONSE)
    /// @retval -1 Connection failed

    SRT_ATR_NODISCARD EConnectStatus processConnectResponse(const CPacket& pkt, CUDTException* eout, EConnectMethod synchro) ATR_NOEXCEPT;

    // This function works in case of HSv5 rendezvous. It changes the state
    // according to the present state and received message type, as well as the
    // INITIATOR/RESPONDER side resolved through cookieContest().
    // The resulting data are:
    // - rsptype: handshake message type that should be sent back to the peer (nothing if URQ_DONE)
    // - needs_extension: the HSREQ/KMREQ or HSRSP/KMRSP extensions should be attached to the handshake message.
    // - RETURNED VALUE: if true, it means a URQ_CONCLUSION message was received with HSRSP/KMRSP extensions and needs HSRSP/KMRSP.
    void rendezvousSwitchState(UDTRequestType& rsptype, bool& needs_extension, bool& needs_hsrsp);
    void cookieContest();

    /// Interpret the incoming handshake packet in order to perform appropriate
    /// rendezvous FSM state transition if needed, and craft the response, serialized
    /// into the packet to be next sent.
    /// @param reqpkt Packet to be written with handshake data
    /// @param response incoming handshake response packet to be interpreted
    /// @param serv_addr incoming packet's address
    /// @param synchro True when this function was called in blocking mode
    /// @param rst Current read status to know if the HS packet was freshly received from the peer, or this is only a periodic update (RST_AGAIN)
    SRT_ATR_NODISCARD EConnectStatus processRendezvous(const CPacket &response, const sockaddr_any& serv_addr, bool synchro, EReadStatus,
            CPacket& reqpkt);
    SRT_ATR_NODISCARD bool prepareConnectionObjects(const CHandShake &hs, HandshakeSide hsd, CUDTException *eout);
    SRT_ATR_NODISCARD EConnectStatus postConnect(const CPacket& response, bool rendezvous, CUDTException* eout, bool synchro) ATR_NOEXCEPT;
    void applyResponseSettings() ATR_NOEXCEPT;
    SRT_ATR_NODISCARD EConnectStatus processAsyncConnectResponse(const CPacket& pkt) ATR_NOEXCEPT;
    SRT_ATR_NODISCARD bool processAsyncConnectRequest(EReadStatus rst, EConnectStatus cst, const CPacket& response, const sockaddr_any& serv_addr);

    void checkUpdateCryptoKeyLen(const char* loghdr, int32_t typefield);

    SRT_ATR_NODISCARD size_t fillSrtHandshake_HSREQ(uint32_t* srtdata, size_t srtlen, int hs_version);
    SRT_ATR_NODISCARD size_t fillSrtHandshake_HSRSP(uint32_t* srtdata, size_t srtlen, int hs_version);
    SRT_ATR_NODISCARD size_t fillSrtHandshake(uint32_t* srtdata, size_t srtlen, int msgtype, int hs_version);

    SRT_ATR_NODISCARD bool createSrtHandshake(int srths_cmd, int srtkm_cmd, const uint32_t* data, size_t datalen,
            CPacket& w_reqpkt, CHandShake& w_hs);

    SRT_ATR_NODISCARD size_t fillHsExtConfigString(uint32_t *pcmdspec, int cmd, const std::string &str);
#if ENABLE_EXPERIMENTAL_BONDING
    SRT_ATR_NODISCARD size_t fillHsExtGroup(uint32_t *pcmdspec);
#endif
    SRT_ATR_NODISCARD size_t fillHsExtKMREQ(uint32_t *pcmdspec, size_t ki);
    SRT_ATR_NODISCARD size_t fillHsExtKMRSP(uint32_t *pcmdspec, const uint32_t *kmdata, size_t kmdata_wordsize);

    SRT_ATR_NODISCARD size_t prepareSrtHsMsg(int cmd, uint32_t* srtdata, size_t size);

    SRT_ATR_NODISCARD bool processSrtMsg(const CPacket *ctrlpkt);
    SRT_ATR_NODISCARD int processSrtMsg_HSREQ(const uint32_t* srtdata, size_t bytelen, uint32_t ts, int hsv);
    SRT_ATR_NODISCARD int processSrtMsg_HSRSP(const uint32_t* srtdata, size_t bytelen, uint32_t ts, int hsv);
    SRT_ATR_NODISCARD bool interpretSrtHandshake(const CHandShake& hs, const CPacket& hspkt, uint32_t* out_data, size_t* out_len);
    SRT_ATR_NODISCARD bool checkApplyFilterConfig(const std::string& cs);

#if ENABLE_EXPERIMENTAL_BONDING
    static CUDTGroup& newGroup(const int); // defined EXCEPTIONALLY in api.cpp for convenience reasons
    // Note: This is an "interpret" function, which should treat the tp as
    // "possibly group type" that might be out of the existing values.
    SRT_ATR_NODISCARD bool interpretGroup(const int32_t grpdata[], size_t data_size, int hsreq_type_cmd);
    SRT_ATR_NODISCARD SRTSOCKET makeMePeerOf(SRTSOCKET peergroup, SRT_GROUP_TYPE tp, uint32_t link_flags);
    void synchronizeWithGroup(CUDTGroup* grp);
#endif

    void updateAfterSrtHandshake(int hsv);

    void updateSrtRcvSettings();
    void updateSrtSndSettings();

    void updateIdleLinkFrom(CUDT* source);

    void checkNeedDrop(bool& bCongestion);

    /// Connect to a UDT entity listening at address "peer", which has sent "hs" request.
    /// @param peer [in] The address of the listening UDT entity.
    /// @param hs [in/out] The handshake information sent by the peer side (in), negotiated value (out).

    void acceptAndRespond(const sockaddr_any& agent, const sockaddr_any& peer, const CPacket& hspkt, CHandShake& hs);
    bool runAcceptHook(CUDT* acore, const sockaddr* peer, const CHandShake& hs, const CPacket& hspkt);

    /// Close the opened UDT entity.

    bool closeInternal();

    /// Request UDT to send out a data block "data" with size of "len".
    /// @param data [in] The address of the application data to be sent.
    /// @param len [in] The size of the data block.
    /// @return Actual size of data sent.

    SRT_ATR_NODISCARD int send(const char* data, int len)
    {
        return sendmsg(data, len, SRT_MSGTTL_INF, false, 0);
    }

    /// Request UDT to receive data to a memory block "data" with size of "len".
    /// @param data [out] data received.
    /// @param len [in] The desired size of data to be received.
    /// @return Actual size of data received.

    SRT_ATR_NODISCARD int recv(char* data, int len);

    /// send a message of a memory block "data" with size of "len".
    /// @param data [out] data received.
    /// @param len [in] The desired size of data to be received.
    /// @param ttl [in] the time-to-live of the message.
    /// @param inorder [in] if the message should be delivered in order.
    /// @param srctime [in] Time when the data were ready to send.
    /// @return Actual size of data sent.

    SRT_ATR_NODISCARD int sendmsg(const char* data, int len, int ttl, bool inorder, int64_t srctime);
    /// Receive a message to buffer "data".
    /// @param data [out] data received.
    /// @param len [in] size of the buffer.
    /// @return Actual size of data received.

    SRT_ATR_NODISCARD int sendmsg2(const char* data, int len, SRT_MSGCTRL& w_m);

    SRT_ATR_NODISCARD int recvmsg(char* data, int len, int64_t& srctime);
    SRT_ATR_NODISCARD int recvmsg2(char* data, int len, SRT_MSGCTRL& w_m);
    SRT_ATR_NODISCARD int receiveMessage(char* data, int len, SRT_MSGCTRL& w_m, int erh = 1 /*throw exception*/);
    SRT_ATR_NODISCARD int receiveBuffer(char* data, int len);

    size_t dropMessage(int32_t seqtoskip);

    /// Request UDT to send out a file described as "fd", starting from "offset", with size of "size".
    /// @param ifs [in] The input file stream.
    /// @param offset [in, out] From where to read and send data; output is the new offset when the call returns.
    /// @param size [in] How many data to be sent.
    /// @param block [in] size of block per read from disk
    /// @return Actual size of data sent.

    SRT_ATR_NODISCARD int64_t sendfile(std::fstream& ifs, int64_t& offset, int64_t size, int block = 366000);

    /// Request UDT to receive data into a file described as "fd", starting from "offset", with expected size of "size".
    /// @param ofs [out] The output file stream.
    /// @param offset [in, out] From where to write data; output is the new offset when the call returns.
    /// @param size [in] How many data to be received.
    /// @param block [in] size of block per write to disk
    /// @return Actual size of data received.

    SRT_ATR_NODISCARD int64_t recvfile(std::fstream& ofs, int64_t& offset, int64_t size, int block = 7320000);

    /// Configure UDT options.
    /// @param optName [in] The enum name of a UDT option.
    /// @param optval [in] The value to be set.
    /// @param optlen [in] size of "optval".

    void setOpt(SRT_SOCKOPT optName, const void* optval, int optlen);

    /// Read UDT options.
    /// @param optName [in] The enum name of a UDT option.
    /// @param optval [in] The value to be returned.
    /// @param optlen [out] size of "optval".

    void getOpt(SRT_SOCKOPT optName, void* optval, int& w_optlen);

#if ENABLE_EXPERIMENTAL_BONDING
    /// Applies the configuration set on the socket.
    /// Any errors in this process are reported by exception.
    SRT_ERRNO applyMemberConfigObject(const SRT_SocketOptionObject& opt);
#endif

    /// read the performance data with bytes counters since bstats() 
    ///  
    /// @param perf [in, out] pointer to a CPerfMon structure to record the performance data.
    /// @param clear [in] flag to decide if the local performance trace should be cleared. 
    /// @param instantaneous [in] flag to request instantaneous data 
    /// instead of moving averages.
    void bstats(CBytePerfMon* perf, bool clear = true, bool instantaneous = false);

    /// Mark sequence contained in the given packet as not lost. This
    /// removes the loss record from both current receiver loss list and
    /// the receiver fresh loss list.
    void unlose(const CPacket& oldpacket);
    void dropFromLossLists(int32_t from, int32_t to);

    void checkSndTimers(Whether2RegenKm regen = DONT_REGEN_KM);
    void handshakeDone()
    {
        m_iSndHsRetryCnt = 0;
    }

    int64_t withOverhead(int64_t basebw)
    {
        return (basebw * (100 + m_iOverheadBW))/100;
    }

    static double Bps2Mbps(int64_t basebw)
    {
        return double(basebw) * 8.0/1000000.0;
    }

    bool stillConnected()
    {
        // Still connected is when:
        // - no "broken" condition appeared (security, protocol error, response timeout)
        return !m_bBroken
            // - still connected (no one called srt_close())
            && m_bConnected
            // - isn't currently closing (srt_close() called, response timeout, shutdown)
            && !m_bClosing;
    }

    int sndSpaceLeft()
    {
        return sndBuffersLeft() * m_iMaxSRTPayloadSize;
    }

    int sndBuffersLeft()
    {
        return m_iSndBufSize - m_pSndBuffer->getCurrBufSize();
    }

    time_point socketStartTime()
    {
        return m_stats.tsStartTime;
    }

    // TSBPD thread main function.
    static void* tsbpd(void* param);

    void updateForgotten(int seqlen, int32_t lastack, int32_t skiptoseqno);

    static loss_seqs_t defaultPacketArrival(void* vself, CPacket& pkt);
    static loss_seqs_t groupPacketArrival(void* vself, CPacket& pkt);

    static CUDTUnited s_UDTUnited;               // UDT global management base

private: // Identification
    CUDTSocket* const m_parent; // temporary, until the CUDTSocket class is merged with CUDT
    SRTSOCKET m_SocketID;                        // UDT socket number
    SRTSOCKET m_PeerID;                          // peer id, for multiplexer

    int m_iMaxSRTPayloadSize;                 // Maximum/regular payload size, in bytes
    size_t m_zOPT_ExpPayloadSize;                    // Expected average payload size (user option)

    // Options
    int m_iMSS;                                  // Maximum Segment Size, in bytes
    bool m_bSynSending;                          // Sending syncronization mode
    bool m_bSynRecving;                          // Receiving syncronization mode
    int m_iFlightFlagSize;                       // Maximum number of packets in flight from the peer side
    int m_iSndBufSize;                           // Maximum UDT sender buffer size
    int m_iRcvBufSize;                           // Maximum UDT receiver buffer size
    linger m_Linger;                             // Linger information on close
    int m_iUDPSndBufSize;                        // UDP sending buffer size
    int m_iUDPRcvBufSize;                        // UDP receiving buffer size
    bool m_bRendezvous;                          // Rendezvous connection mode

    duration m_tdConnTimeOut;    // connect timeout in milliseconds
    bool m_bDriftTracer;
    int m_iSndTimeOut;                           // sending timeout in milliseconds
    int m_iRcvTimeOut;                           // receiving timeout in milliseconds
    bool m_bReuseAddr;                           // reuse an exiting port or not, for UDP multiplexer
    int64_t m_llMaxBW;                           // maximum data transfer rate (threshold)
    int m_iIpTTL;
    int m_iIpToS;
#ifdef SRT_ENABLE_BINDTODEVICE
    std::string m_BindToDevice;
#endif
    // These fields keep the options for encryption
    // (SRTO_PASSPHRASE, SRTO_PBKEYLEN). Crypto object is
    // created later and takes values from these.
    HaiCrypt_Secret m_CryptoSecret;
    int m_iSndCryptoKeyLen;

    // XXX Consider removing. The m_bDataSender stays here
    // in order to maintain the HS side selection in HSv4.
    bool m_bDataSender;

    // HSv4 (legacy handshake) support)
    time_point m_tsSndHsLastTime;	    //Last SRT handshake request time
    int      m_iSndHsRetryCnt;       //SRT handshake retries left

    bool m_bMessageAPI;
    bool m_bOPT_TsbPd;               // Whether AGENT will do TSBPD Rx (whether peer does, is not agent's problem)
    int m_iOPT_TsbPdDelay;           // Agent's Rx latency
    int m_iOPT_PeerTsbPdDelay;       // Peer's Rx latency for the traffic made by Agent's Tx.
    bool m_bOPT_TLPktDrop;           // Whether Agent WILL DO TLPKTDROP on Rx.
    int m_iOPT_SndDropDelay;         // Extra delay when deciding to snd-drop for TLPKTDROP, -1 to off
    bool m_bOPT_StrictEncryption;    // Off by default. When on, any connection other than nopw-nopw & pw1-pw1 is rejected.
    int m_OPT_GroupConnect;
    std::string m_sStreamName;
    int m_iOPT_PeerIdleTimeout;      // Timeout for hearing anything from the peer.
    uint32_t m_uOPT_StabilityTimeout;
    int m_iOPT_RetransmitAlgo;

    int m_iTsbPdDelay_ms;                           // Rx delay to absorb burst in milliseconds
    int m_iPeerTsbPdDelay_ms;                       // Tx delay that the peer uses to absorb burst in milliseconds
    bool m_bTLPktDrop;                           // Enable Too-late Packet Drop
    int64_t m_llInputBW;                         // Input stream rate (bytes/sec)
                                                 // 0: use internally estimated input bandwidth
    int m_iOverheadBW;                           // Percent above input stream rate (applies if m_llMaxBW == 0)
    bool m_bRcvNakReport;                        // Enable Receiver Periodic NAK Reports
    int m_iIpV6Only;                             // IPV6_V6ONLY option (-1 if not set)
#if ENABLE_EXPERIMENTAL_BONDING
    SRT_GROUP_TYPE m_HSGroupType;   // group type about-to-be-set in the handshake
#endif

private:
    UniquePtr<CCryptoControl> m_pCryptoControl;                            // congestion control SRT class (small data extension)
    CCache<CInfoBlock>* m_pCache;                // network information cache

    // Congestion control
    std::vector<EventSlot> m_Slots[TEV_E_SIZE];
    SrtCongestion m_CongCtl;

    // Packet filtering
    PacketFilter m_PacketFilter;
    std::string m_OPT_PktFilterConfigString;
    SRT_ARQLevel m_PktFilterRexmitLevel;
    std::string m_sPeerPktFilterConfigString;

    // Attached tool function
    void EmitSignal(ETransmissionEvent tev, EventVariant var);

    // Internal state
    volatile bool m_bListening;                  // If the UDT entit is listening to connection
    volatile bool m_bConnecting;                 // The short phase when connect() is called but not yet completed
    volatile bool m_bConnected;                  // Whether the connection is on or off
    volatile bool m_bClosing;                    // If the UDT entity is closing
    volatile bool m_bShutdown;                   // If the peer side has shutdown the connection
    volatile bool m_bBroken;                     // If the connection has been broken
    volatile bool m_bPeerHealth;                 // If the peer status is normal
    volatile int m_RejectReason;
    bool m_bOpened;                              // If the UDT entity has been opened
    int m_iBrokenCounter;                        // a counter (number of GC checks) to let the GC tag this socket as disconnected

    int m_iEXPCount;                             // Expiration counter
    int m_iBandwidth;                            // Estimated bandwidth, number of packets per second
    int m_iRTT;                                  // RTT, in microseconds
    int m_iRTTVar;                               // RTT variance
    int m_iDeliveryRate;                         // Packet arrival rate at the receiver side
    int m_iByteDeliveryRate;                     // Byte arrival rate at the receiver side


    CHandShake m_ConnReq;                        // connection request
    CHandShake m_ConnRes;                        // connection response
    CHandShake::RendezvousState m_RdvState;      // HSv5 rendezvous state
    HandshakeSide m_SrtHsSide;                   // HSv5 rendezvous handshake side resolved from cookie contest (DRAW if not yet resolved)

private: // Sending related data
    CSndBuffer* m_pSndBuffer;                    // Sender buffer
    CSndLossList* m_pSndLossList;                // Sender loss list
    CPktTimeWindow<16, 16> m_SndTimeWindow;      // Packet sending time window

    /*volatile*/ duration m_tdSendInterval;      // Inter-packet time, in CPU clock cycles

    /*volatile*/ duration m_tdSendTimeDiff;      // aggregate difference in inter-packet sending time

    volatile int m_iFlowWindowSize;              // Flow control window size
    volatile double m_dCongestionWindow;         // congestion window size

private: // Timers
    /*volatile*/ time_point m_tsNextACKTime;    // Next ACK time, in CPU clock cycles, same below
    /*volatile*/ time_point m_tsNextNAKTime;    // Next NAK time

    /*volatile*/ duration   m_tdACKInterval;    // ACK interval
    /*volatile*/ duration   m_tdNAKInterval;    // NAK interval
    /*volatile*/ time_point m_tsLastRspTime;    // time stamp of last response from the peer
    /*volatile*/ time_point m_tsLastRspAckTime; // time stamp of last ACK from the peer
    /*volatile*/ time_point m_tsLastSndTime;    // time stamp of last data/ctrl sent (in system ticks)
    time_point m_tsLastWarningTime;             // Last time that a warning message is sent
    time_point m_tsLastReqTime;                 // last time when a connection request is sent
    time_point m_tsRcvPeerStartTime;
    time_point m_tsLingerExpiration;            // Linger expiration time (for GC to close a socket with data in sending buffer)
    time_point m_tsLastAckTime;                 // Timestamp of last ACK
    duration m_tdMinNakInterval;                // NAK timeout lower bound; too small value can cause unnecessary retransmission
    duration m_tdMinExpInterval;                // timeout lower bound threshold: too small timeout can cause problem

    int m_iPktCount;                          // packet counter for ACK
    int m_iLightACKCount;                     // light ACK counter

    time_point m_tsNextSendTime;     // scheduled time of next packet sending

    volatile int32_t m_iSndLastFullAck;          // Last full ACK received
    volatile int32_t m_iSndLastAck;              // Last ACK received

    // NOTE: m_iSndLastDataAck is the value strictly bound to the CSndBufer object (m_pSndBuffer)
    // and this is the sequence number that refers to the block at position [0]. Upon acknowledgement,
    // this value is shifted to the acknowledged position, and the blocks are removed from the
    // m_pSndBuffer buffer up to excluding this sequence number.
    // XXX CONSIDER removing this field and give up the maintenance of this sequence number
    // to the sending buffer. This way, extraction of an old packet for retransmission should
    // require only the lost sequence number, and how to find the packet with this sequence
    // will be up to the sending buffer.
    volatile int32_t m_iSndLastDataAck;          // The real last ACK that updates the sender buffer and loss list
    volatile int32_t m_iSndCurrSeqNo;            // The largest sequence number that HAS BEEN SENT
    volatile int32_t m_iSndNextSeqNo;            // The sequence number predicted to be placed at the currently scheduled packet

    // Note important differences between Curr and Next fields:
    // - m_iSndCurrSeqNo: this is used by SRT:SndQ:worker thread and it's operated from CUDT::packData
    //   function only. This value represents the sequence number that has been stamped on a packet directly
    //   before it is sent over the network.
    // - m_iSndNextSeqNo: this is used by the user's thread and it's operated from CUDT::sendmsg2
    //   function only. This value represents the sequence number that is PREDICTED to be stamped on the
    //   first block out of the block series that will be scheduled for later sending over the network
    //   out of the data passed in this function. For a special case when the length of the data is
    //   short enough to be passed in one UDP packet (always the case for live mode), this value is
    //   always increased by one in this call, otherwise it will be increased by the number of blocks
    //   scheduled for sending.

    int32_t m_iSndLastAck2;                      // Last ACK2 sent back
    time_point m_SndLastAck2Time;                // The time when last ACK2 was sent back
    void setInitialSndSeq(int32_t isn)
    {
        m_iSndLastAck = isn;
        m_iSndLastDataAck = isn;
        m_iSndLastFullAck = isn;
        m_iSndCurrSeqNo = CSeqNo::decseq(isn);
        m_iSndNextSeqNo = isn;
        m_iSndLastAck2 = isn;
    }

    void setInitialRcvSeq(int32_t isn)
    {
        m_iRcvLastAck = isn;
#ifdef ENABLE_LOGGING
        m_iDebugPrevLastAck = m_iRcvLastAck;
#endif
        m_iRcvLastSkipAck = m_iRcvLastAck;
        m_iRcvLastAckAck = isn;
        m_iRcvCurrSeqNo = CSeqNo::decseq(isn);
    }

    int32_t m_iISN;                              // Initial Sequence Number
    bool m_bPeerTsbPd;                           // Peer accept TimeStamp-Based Rx mode
    bool m_bPeerTLPktDrop;                       // Enable sender late packet dropping
    bool m_bPeerNakReport;                       // Sender's peer (receiver) issues Periodic NAK Reports
    bool m_bPeerRexmitFlag;                      // Receiver supports rexmit flag in payload packets
    int32_t m_iReXmitCount;                      // Re-Transmit Count since last ACK

private: // Receiving related data
    CRcvBuffer* m_pRcvBuffer;                    //< Receiver buffer
    CRcvLossList* m_pRcvLossList;                //< Receiver loss list
    std::deque<CRcvFreshLoss> m_FreshLoss;       //< Lost sequence already added to m_pRcvLossList, but not yet sent UMSG_LOSSREPORT for.
    int m_iReorderTolerance;                     //< Current value of dynamic reorder tolerance
    int m_iMaxReorderTolerance;                  //< Maximum allowed value for dynamic reorder tolerance
    int m_iConsecEarlyDelivery;                  //< Increases with every OOO packet that came <TTL-2 time, resets with every increased reorder tolerance
    int m_iConsecOrderedDelivery;                //< Increases with every packet coming in order or retransmitted, resets with every out-of-order packet

    CACKWindow<1024> m_ACKWindow;                //< ACK history window
    CPktTimeWindow<16, 64> m_RcvTimeWindow;      //< Packet arrival time window

    int32_t m_iRcvLastAck;                       //< Last sent ACK
#ifdef ENABLE_LOGGING
    int32_t m_iDebugPrevLastAck;
#endif
    int32_t m_iRcvLastSkipAck;                   // Last dropped sequence ACK
    int32_t m_iRcvLastAckAck;                    // Last sent ACK that has been acknowledged
    int32_t m_iAckSeqNo;                         // Last ACK sequence number
    int32_t m_iRcvCurrSeqNo;                     // Largest received sequence number
    int32_t m_iRcvCurrPhySeqNo;                  // Same as m_iRcvCurrSeqNo, but physical only (disregarding a filter)

    int32_t m_iPeerISN;                          // Initial Sequence Number of the peer side

    uint32_t m_lSrtVersion;
    uint32_t m_lMinimumPeerSrtVersion;
    uint32_t m_lPeerSrtVersion;
    uint32_t m_lPeerSrtFlags;

    bool m_bTsbPd;                               // Peer sends TimeStamp-Based Packet Delivery Packets 
    bool m_bGroupTsbPd;                          // TSBPD should be used for GROUP RECEIVER instead.

    srt::sync::CThread m_RcvTsbPdThread;         // Rcv TsbPD Thread handle
    srt::sync::Condition m_RcvTsbPdCond;         // TSBPD signals if reading is ready
    bool m_bTsbPdAckWakeup;                      // Signal TsbPd thread on Ack sent

    CallbackHolder<srt_listen_callback_fn> m_cbAcceptHook;
    CallbackHolder<srt_connect_callback_fn> m_cbConnectHook;

    // FORWARDER
public:
    static int installAcceptHook(SRTSOCKET lsn, srt_listen_callback_fn* hook, void* opaq);
    static int installConnectHook(SRTSOCKET lsn, srt_connect_callback_fn* hook, void* opaq);
private:
    void installAcceptHook(srt_listen_callback_fn* hook, void* opaq)
    {
        m_cbAcceptHook.set(opaq, hook);
    }

    void installConnectHook(srt_connect_callback_fn* hook, void* opaq)
    {
        m_cbConnectHook.set(opaq, hook);
    }


private: // synchronization: mutexes and conditions
    srt::sync::Mutex m_ConnectionLock;           // used to synchronize connection operation

    srt::sync::Condition m_SendBlockCond;        // used to block "send" call
    srt::sync::Mutex m_SendBlockLock;            // lock associated to m_SendBlockCond

    srt::sync::Mutex m_RcvBufferLock;            // Protects the state of the m_pRcvBuffer
    // Protects access to m_iSndCurrSeqNo, m_iSndLastAck
    srt::sync::Mutex m_RecvAckLock;              // Protects the state changes while processing incomming ACK (SRT_EPOLL_OUT)

    srt::sync::Condition m_RecvDataCond;         // used to block "recv" when there is no data
    srt::sync::Mutex m_RecvDataLock;             // lock associated to m_RecvDataCond

    srt::sync::Mutex m_SendLock;                 // used to synchronize "send" call
    srt::sync::Mutex m_RecvLock;                 // used to synchronize "recv" call, protects TSBPD drift updates (CRcvBuffer::isRcvDataReady())
    srt::sync::Mutex m_RcvLossLock;              // Protects the receiver loss list (access: CRcvQueue::worker, CUDT::tsbpd)
    srt::sync::Mutex m_StatsLock;                // used to synchronize access to trace statistics

    void initSynch();
    void destroySynch();
    void releaseSynch();

private: // Common connection Congestion Control setup
    // This can fail only when it failed to create a congctl
    // which only may happen when the congctl list is extended 
    // with user-supplied congctl modules, not a case so far.
    SRT_ATR_NODISCARD
    SRT_REJECT_REASON setupCC();

    // for updateCC it's ok to discard the value. This returns false only if
    // the congctl isn't created, and this can be prevented from.
    bool updateCC(ETransmissionEvent, const EventVariant arg);

    // Failure to create the crypter means that an encrypted
    // connection should be rejected if ENFORCEDENCRYPTION is on.
    SRT_ATR_NODISCARD
    bool createCrypter(HandshakeSide side, bool bidi);

private: // Generation and processing of packets
    void sendCtrl(UDTMessageType pkttype, const int32_t* lparam = NULL, void* rparam = NULL, int size = 0);

    void processCtrl(const CPacket& ctrlpkt);
    void sendLossReport(const std::vector< std::pair<int32_t, int32_t> >& losslist);
    void processCtrlAck(const CPacket& ctrlpkt, const time_point &currtime);
    void processCtrlLossReport(const CPacket& ctrlpkt);

    ///
    /// @param ackdata_seqno    sequence number of a data packet being acknowledged
    void updateSndLossListOnACK(int32_t ackdata_seqno);

    /// Pack a packet from a list of lost packets.
    ///
    /// @param packet [in, out] a packet structure to fill
    /// @param origintime [in, out] origin timestamp of the packet
    ///
    /// @return payload size on success, <=0 on failure
    int packLostData(CPacket &packet, time_point &origintime);

    /// Pack in CPacket the next data to be send.
    ///
    /// @param packet [in, out] a CPacket structure to fill
    ///
    /// @return A pair of values is returned (payload, timestamp).
    ///         The payload tells the size of the payload, packed in CPacket.
    ///         The timestamp is the full source/origin timestamp of the data.
    ///         If payload is <= 0, consider the timestamp value invalid.
    std::pair<int, time_point> packData(CPacket& packet);

    int processData(CUnit* unit);
    void processClose();

    /// Process the request after receiving the handshake from caller.
    /// The @a packet param is passed here as non-const because this function
    /// will need to make a temporary back-and-forth endian swap; it doesn't intend to
    /// modify the object permanently.
    /// @param addr source address from where the request came
    /// @param packet contents of the packet
    /// @return URQ code, possibly containing reject reason
    int processConnectRequest(const sockaddr_any& addr, CPacket& packet);
    static void addLossRecord(std::vector<int32_t>& lossrecord, int32_t lo, int32_t hi);
    int32_t bake(const sockaddr_any& addr, int32_t previous_cookie = 0, int correction = 0);
    int32_t ackDataUpTo(int32_t seq);
    void handleKeepalive(const char* data, size_t lenghth);

private: // Trace
    struct CoreStats
    {
        time_point tsStartTime;                 // timestamp when the UDT entity is started
        int64_t sentTotal;                  // total number of sent data packets, including retransmissions
        int64_t sentUniqTotal;              // total number of sent data packets, excluding rexmit and filter control
        int64_t recvTotal;                  // total number of received packets
        int64_t recvUniqTotal;              // total number of received and delivered packets
        int sndLossTotal;                   // total number of lost packets (sender side)
        int rcvLossTotal;                   // total number of lost packets (receiver side)
        int retransTotal;                   // total number of retransmitted packets
        int sentACKTotal;                   // total number of sent ACK packets
        int recvACKTotal;                   // total number of received ACK packets
        int sentNAKTotal;                   // total number of sent NAK packets
        int recvNAKTotal;                   // total number of received NAK packets
        int sndDropTotal;
        int rcvDropTotal;
        uint64_t bytesSentTotal;            // total number of bytes sent,  including retransmissions
        uint64_t bytesSentUniqTotal;        // total number of bytes sent,  including retransmissions
        uint64_t bytesRecvTotal;            // total number of received bytes
        uint64_t bytesRecvUniqTotal;        // total number of received bytes
        uint64_t rcvBytesLossTotal;         // total number of loss bytes (estimate)
        uint64_t bytesRetransTotal;         // total number of retransmitted bytes
        uint64_t sndBytesDropTotal;
        uint64_t rcvBytesDropTotal;
        int m_rcvUndecryptTotal;
        uint64_t m_rcvBytesUndecryptTotal;

        int sndFilterExtraTotal;
        int rcvFilterExtraTotal;
        int rcvFilterSupplyTotal;
        int rcvFilterLossTotal;

        int64_t m_sndDurationTotal;         // total real time for sending

        time_point tsLastSampleTime;            // last performance sample time
        int64_t traceSent;                  // number of packets sent in the last trace interval
        int64_t traceSentUniq;              // number of original packets sent in the last trace interval
        int64_t traceRecv;                  // number of packets received in the last trace interval
        int64_t traceRecvUniq;              // number of packets received AND DELIVERED in the last trace interval
        int traceSndLoss;                   // number of lost packets in the last trace interval (sender side)
        int traceRcvLoss;                   // number of lost packets in the last trace interval (receiver side)
        int traceRetrans;                   // number of retransmitted packets in the last trace interval
        int sentACK;                        // number of ACKs sent in the last trace interval
        int recvACK;                        // number of ACKs received in the last trace interval
        int sentNAK;                        // number of NAKs sent in the last trace interval
        int recvNAK;                        // number of NAKs received in the last trace interval
        int traceSndDrop;
        int traceRcvDrop;
        int traceRcvRetrans;
        int traceReorderDistance;
        double traceBelatedTime;
        int64_t traceRcvBelated;
        uint64_t traceBytesSent;            // number of bytes sent in the last trace interval
        uint64_t traceBytesSentUniq;        // number of bytes sent in the last trace interval
        uint64_t traceBytesRecv;            // number of bytes sent in the last trace interval
        uint64_t traceBytesRecvUniq;        // number of bytes sent in the last trace interval
        uint64_t traceRcvBytesLoss;         // number of bytes bytes lost in the last trace interval (estimate)
        uint64_t traceBytesRetrans;         // number of bytes retransmitted in the last trace interval
        uint64_t traceSndBytesDrop;
        uint64_t traceRcvBytesDrop;
        int traceRcvUndecrypt;
        uint64_t traceRcvBytesUndecrypt;

        int sndFilterExtra;
        int rcvFilterExtra;
        int rcvFilterSupply;
        int rcvFilterLoss;

        int64_t sndDuration;                // real time for sending
        time_point sndDurationCounter;         // timers to record the sending Duration
    } m_stats;

public:
    static const int SELF_CLOCK_INTERVAL = 64;  // ACK interval for self-clocking
    static const int SEND_LITE_ACK = sizeof(int32_t); // special size for ack containing only ack seq
    static const int PACKETPAIR_MASK = 0xF;

    static const size_t MAX_SID_LENGTH = 512;

private: // Timers functions
    time_point m_tsTmpActiveTime;  // time since temporary activated, or 0 if not temporary activated
    time_point m_tsUnstableSince;  // time since unexpected ACK delay experienced, or 0 if link seems healthy
    
    static const int BECAUSE_NO_REASON = 0, // NO BITS
                     BECAUSE_ACK       = 1 << 0,
                     BECAUSE_LITEACK   = 1 << 1,
                     BECAUSE_NAKREPORT = 1 << 2,
                     LAST_BECAUSE_BIT  =      3;

    void checkTimers();
    void considerLegacySrtHandshake(const time_point &timebase);
    int checkACKTimer (const time_point& currtime);
    int checkNAKTimer(const time_point& currtime);
    bool checkExpTimer (const time_point& currtime, int check_reason);  // returns true if the connection is expired
    void checkRexmitTimer(const time_point& currtime);

public: // For the use of CCryptoControl
    // HaiCrypt configuration
    unsigned int m_uKmRefreshRatePkt;
    unsigned int m_uKmPreAnnouncePkt;


private: // for UDP multiplexer
    CSndQueue* m_pSndQueue;         // packet sending queue
    CRcvQueue* m_pRcvQueue;         // packet receiving queue
    sockaddr_any m_PeerAddr;        // peer address
    uint32_t m_piSelfIP[4];         // local UDP IP address
    CSNode* m_pSNode;               // node information for UDT list used in snd queue
    CRNode* m_pRNode;               // node information for UDT list used in rcv queue

public: // For SrtCongestion
    const CSndQueue* sndQueue() { return m_pSndQueue; }
    const CRcvQueue* rcvQueue() { return m_pRcvQueue; }

private: // for epoll
    std::set<int> m_sPollID;                     // set of epoll ID to trigger
    void addEPoll(const int eid);
    void removeEPollEvents(const int eid);
    void removeEPollID(const int eid);
};


#endif

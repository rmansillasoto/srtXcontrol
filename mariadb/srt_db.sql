-- phpMyAdmin SQL Dump
-- version 5.1.0
-- https://www.phpmyadmin.net/
--
-- Servidor: bbdd
-- Tiempo de generación: 17-03-2021 a las 10:50:27
-- Versión del servidor: 10.4.13-MariaDB-1:10.4.13+maria~focal
-- Versión de PHP: 7.4.15

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Base de datos: `srt_db`
--

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `rx`
--

CREATE TABLE `rx` (
  `id` int(255) NOT NULL,
  `mode` varchar(255) NOT NULL,
  `tx_ip` varchar(255) NOT NULL,
  `listen_port` int(20) NOT NULL,
  `udp_out` varchar(255) NOT NULL,
  `latency` int(20) NOT NULL,
  `srt_bw` int(20) NOT NULL,
  `service_name` varchar(255) NOT NULL,
  `container_id` varchar(255) DEFAULT NULL,
  `status` tinyint(1) NOT NULL DEFAULT 0,
  `link_rtt` float NOT NULL DEFAULT 0,
  `link_bandwidth` float NOT NULL DEFAULT 0,
  `recv_packets` int(255) NOT NULL DEFAULT 0,
  `recv_packetsUnique` int(255) NOT NULL DEFAULT 0,
  `recv_packetsLost` int(255) NOT NULL DEFAULT 0,
  `recv_packetsDropped` int(255) NOT NULL DEFAULT 0,
  `recv_packetsRetransmitted` int(255) NOT NULL DEFAULT 0,
  `recv_packetsBelated` int(255) NOT NULL DEFAULT 0,
  `recv_packetsFilterExtra` int(255) NOT NULL DEFAULT 0,
  `recv_packetsFilterSupply` int(255) NOT NULL DEFAULT 0,
  `recv_packetsFilterLoss` int(255) NOT NULL DEFAULT 0,
  `recv_bytes` bigint(255) NOT NULL DEFAULT 0,
  `recv_bytesUnique` bigint(255) NOT NULL DEFAULT 0,
  `recv_bytesLost` bigint(255) NOT NULL DEFAULT 0,
  `recv_bytesDropped` bigint(255) NOT NULL DEFAULT 0,
  `recv_mbitRate` float NOT NULL DEFAULT 0,
  `line_loss` float NOT NULL DEFAULT 0,
  `connected` int(1) NOT NULL DEFAULT 0,
  `reconnections` int(255) NOT NULL DEFAULT 0,
  `link_maxbandwidth` float NOT NULL DEFAULT 0,
  `passphrase` varchar(40) NOT NULL,
  `pbkeylen` int(10) NOT NULL DEFAULT 16,
  `tlpktdrop` tinyint(1) NOT NULL DEFAULT 0,
  `ttl` int(2) NOT NULL DEFAULT 24,
  `packetfilter` tinyint(1) DEFAULT 0,
  `fec_cols` int(2) DEFAULT 10,
  `fec_rows` int(2) DEFAULT 5,
  `fec_layout` varchar(20) DEFAULT 'staircase',
  `fec_arq` varchar(10) DEFAULT 'always',
  `congestion` varchar(10) NOT NULL DEFAULT 'live',
  `transtype` varchar(10) NOT NULL DEFAULT 'live',
  `group_connect` tinyint(2) NOT NULL DEFAULT 0,
  `group_type` varchar(20) NOT NULL DEFAULT 'backup',
  `group_ip1` varchar(20) NOT NULL DEFAULT '',
  `group_ip2` varchar(20) NOT NULL DEFAULT '',
  `ip1_weight` int(10) NOT NULL DEFAULT 60,
  `ip2_weight` int(10) NOT NULL DEFAULT 40
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Volcado de datos para la tabla `rx`
--

INSERT INTO `rx` (`id`, `mode`, `tx_ip`, `listen_port`, `udp_out`, `latency`, `srt_bw`, `service_name`, `container_id`, `status`, `link_rtt`, `link_bandwidth`, `recv_packets`, `recv_packetsUnique`, `recv_packetsLost`, `recv_packetsDropped`, `recv_packetsRetransmitted`, `recv_packetsBelated`, `recv_packetsFilterExtra`, `recv_packetsFilterSupply`, `recv_packetsFilterLoss`, `recv_bytes`, `recv_bytesUnique`, `recv_bytesLost`, `recv_bytesDropped`, `recv_mbitRate`, `line_loss`, `connected`, `reconnections`, `link_maxbandwidth`, `passphrase`, `pbkeylen`, `tlpktdrop`, `ttl`, `packetfilter`, `fec_cols`, `fec_rows`, `fec_layout`, `fec_arq`, `congestion`, `transtype`, `group_connect`, `group_type`, `group_ip1`, `group_ip2`, `ip1_weight`, `ip2_weight`) VALUES
(5, 'caller', '80.84.129.34', 7005, '224.10.10.149:5009', 700, 12000, 'Test_13', '', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'mansi11as0t0', 16, 0, 24, 0, 10, 5, 'staircase', 'always', 'live', 'live', 0, 'backup', '', '', 60, 40);

-- --------------------------------------------------------

--
-- Estructura de tabla para la tabla `tx`
--

CREATE TABLE `tx` (
  `id` int(20) NOT NULL,
  `mode` varchar(255) NOT NULL,
  `rx_ip` varchar(255) NOT NULL,
  `listen_port` int(20) NOT NULL,
  `udp_in` varchar(255) NOT NULL,
  `latency` int(20) NOT NULL,
  `srt_bw` int(20) NOT NULL,
  `service_name` varchar(255) NOT NULL,
  `container_id` varchar(255) DEFAULT NULL,
  `status` tinyint(1) NOT NULL DEFAULT 0,
  `link_rtt` float NOT NULL DEFAULT 0,
  `link_bandwidth` float NOT NULL DEFAULT 0,
  `send_packets` int(255) NOT NULL DEFAULT 0,
  `send_packetsUnique` int(255) NOT NULL DEFAULT 0,
  `send_packetsLost` int(255) NOT NULL DEFAULT 0,
  `send_packetsDropped` int(255) NOT NULL DEFAULT 0,
  `send_packetsRetransmitted` int(255) NOT NULL DEFAULT 0,
  `send_packetsFilterExtra` int(255) NOT NULL DEFAULT 0,
  `send_bytes` bigint(255) NOT NULL DEFAULT 0,
  `send_bytesUnique` bigint(255) NOT NULL DEFAULT 0,
  `send_bytesDropped` bigint(255) NOT NULL DEFAULT 0,
  `send_mbitRate` float NOT NULL DEFAULT 0,
  `line_loss` float NOT NULL DEFAULT 0,
  `connected` int(1) NOT NULL DEFAULT 0,
  `reconnections` int(255) NOT NULL DEFAULT 0,
  `link_maxbandwidth` float NOT NULL DEFAULT 0,
  `passphrase` varchar(40) NOT NULL DEFAULT '',
  `pbkeylen` int(10) NOT NULL DEFAULT 16,
  `groupconnect` tinyint(2) NOT NULL DEFAULT 0,
  `input_bw` int(20) NOT NULL DEFAULT 0,
  `overhead` int(20) NOT NULL DEFAULT 25,
  `packetfilter` tinyint(2) NOT NULL DEFAULT 0,
  `fec_cols` int(10) NOT NULL DEFAULT 10,
  `fec_rows` int(10) NOT NULL DEFAULT 5,
  `fec_layout` varchar(20) NOT NULL DEFAULT 'staircase',
  `fec_arq` varchar(20) NOT NULL DEFAULT 'always',
  `congestion` varchar(20) NOT NULL DEFAULT 'live',
  `transtype` varchar(20) NOT NULL DEFAULT 'live'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

--
-- Índices para tablas volcadas
--

--
-- Indices de la tabla `rx`
--
ALTER TABLE `rx`
  ADD PRIMARY KEY (`id`);

--
-- Indices de la tabla `tx`
--
ALTER TABLE `tx`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT de las tablas volcadas
--

--
-- AUTO_INCREMENT de la tabla `rx`
--
ALTER TABLE `rx`
  MODIFY `id` int(255) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=6;

--
-- AUTO_INCREMENT de la tabla `tx`
--
ALTER TABLE `tx`
  MODIFY `id` int(20) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=6;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;


# Basic OpenFlow Software Switch (BOFUSS) with Enhanced Packet Sampling

This is an [OpenFlow 1.3][ofp13] compatible user-space software switch implementation with **enhanced packet sampling capabilities**. The code is based on the [Ericsson TrafficLab 1.1 softswitch implementation][ericssonsw11], with changes in the forwarding plane to support OpenFlow 1.3 and advanced packet sampling features.

### [A paper that describes the basic architecture, selected use cases and a few benchmarks is available on Arxiv](https://arxiv.org/abs/1901.06699). 
If you use the switch for academic purpuses, please consider the use of this citation.

```
@article{fernandes2020road,
  title={The road to BOFUSS: The basic OpenFlow userspace software switch},
  author={Fernandes, Eder Le{\~a}o and Rojas, Elisa and Alvarez-Horcajo, Joaquin and Kis, Zolt{\`a}n Lajos and Sanvito, Davide and Bonelli, Nicola and Cascone, Carmelo and Rothenberg, Christian Esteve},
  journal={Journal of Network and Computer Applications},
  pages={102685},
  year={2020},
  publisher={Elsevier}
}
```

## 🆕 New Feature: Packet Sampling Extension

The switch now supports **per-flow packet sampling** with the following capabilities:

- **Flow-based sampling**: Sample packets based on flow table entries
- **Configurable probability**: Adjustable sampling probability (0-100%)
- **Rate limiting**: Token bucket algorithm for precise rate control
- **Independent sending**: Sampled packets sent to dedicated receiver servers
- **Rich metadata**: Timestamp, input port, and packet length information
- **Standard protocol**: Uses OpenFlow action extension (no experimenter ID required)

### Key Benefits
- **Network monitoring**: Real-time traffic analysis without affecting performance
- **Security analysis**: Capture suspicious traffic patterns
- **Quality of Service**: Monitor specific flow characteristics
- **Research and development**: Flexible packet capture for academic purposes

## Enhanced Components with Sampling Support

The following components are available in this package:

* `ofdatapath`: the switch implementation **with packet sampling capabilities**
* `ofprotocol`: secure channel for connecting the switch to the controller
* `oflib`: a library for converting to/from 1.3 wire format
* `dpctl`: a tool for configuring the switch from the console
* `packet_receiver`: **NEW** - UDP server for receiving sampled packets
* `sample_flow_mod`: **NEW** - Tool for installing flow entries with sampling actions

## 🚀 Quick Start with Packet Sampling

### Step 1: Start the Packet Receiver

First, start the packet receiver on your monitoring server:

```bash
# Start receiver on port 9999, save packets to sampled.pcap
$ utilities/packet_receiver 9999 sampled.pcap
```

The receiver will display real-time statistics:
```
Packet receiver started on port 9999
Saving packets to: sampled.pcap
Press Ctrl+C to stop

[2024-01-15 10:30:25.123456] Packet #1: port=1, length=64 bytes, from 192.168.1.100:56342
[2024-01-15 10:30:25.234567] Packet #2: port=2, length=128 bytes, from 192.168.1.100:56342
```

### Step 2: Install Sampling Flow Entries

Use `sample_flow_mod` to install flow entries with sampling actions:

```bash
# Sample all traffic from port 1 at 50% probability, max 100 packets/sec
$ utilities/sample_flow_mod tcp:localhost:6633 \
    -i 1 -o 2 \
    --sample-prob 500000 \
    --max-rate 100 \
    --server-ip 192.168.1.100 \
    --server-port 9999

# Sample specific IP traffic at 10% probability
$ utilities/sample_flow_mod tcp:localhost:6633 \
    -s 10.0.0.1 -d 10.0.0.2 \
    --sample-prob 100000 \
    --max-rate 10 \
    --server-ip 192.168.1.100
```

### Step 3: Monitor Sampled Traffic

The receiver will capture and display sampled packets in real-time, saving them to a pcap file for later analysis with tools like Wireshark.

## 📋 Detailed Usage Guide

### Packet Receiver (`packet_receiver`)

The packet receiver is a standalone UDP server that receives sampled packets from the switch.

#### Basic Usage
```bash
# Start receiver on default port 9999
$ utilities/packet_receiver 9999

# Specify custom pcap filename
$ utilities/packet_receiver 9999 my_sampled_packets.pcap
```

#### Command Line Options
- **Port**: UDP port to listen on (required)
- **Pcap file**: Output filename (default: `sampled_packets.pcap`)

#### Output Example
```
Packet receiver started on port 9999
Saving packets to: sampled_packets.pcap
Press Ctrl+C to stop

[2024-01-15 10:30:25.123456] Packet #1: port=1, length=64 bytes, from 192.168.1.100:56342
[2024-01-15 10:30:25.234567] Packet #2: port=2, length=128 bytes, from 192.168.1.100:56342
```

### Flow Entry Manager (`sample_flow_mod`)

Install flow entries with packet sampling actions to the switch.

#### Basic Syntax
```bash
utilities/sample_flow_mod <switch-host:port> [OPTIONS]
```

#### Common Options
| Option | Description | Example |
|--------|-------------|---------|
| `-t, --table` | Table ID | `-t 0` |
| `-p, --priority` | Flow priority | `-p 1000` |
| `-i, --in-port` | Match input port | `-i 1` |
| `-s, --src-ip` | Match source IP | `-s 10.0.0.1` |
| `-d, --dst-ip` | Match destination IP | `-d 10.0.0.2` |
| `--src-mac` | Match source MAC | `--src-mac 00:11:22:33:44:55` |
| `--dst-mac` | Match destination MAC | `--dst-mac 00:11:22:33:44:55` |
| `--eth-type` | Match Ethernet type | `--eth-type 0x0800` (IPv4) |
| `--ip-proto` | Match IP protocol | `--ip-proto 6` (TCP) |
| `--sample-prob` | Sampling probability | `--sample-prob 500000` (50%) |
| `--max-rate` | Max sampling rate | `--max-rate 100` (100 pps) |
| `--server-ip` | Receiver server IP | `--server-ip 192.168.1.100` |
| `--server-port` | Receiver server port | `--server-port 9999` |
| `-o, --output` | Output port | `-o 2` |
| `-a, --add` | Add flow (default) | `-a` |
| `-D, --delete` | Delete matching flows | `-D` |

#### Practical Examples

**Example 1: Basic Port Sampling**
```bash
# Sample 50% of traffic from port 1, forward to port 2
$ utilities/sample_flow_mod tcp:localhost:6633 \
    -i 1 -o 2 \
    --sample-prob 500000 \
    --max-rate 100 \
    --server-ip 192.168.1.100 \
    --server-port 9999
```

**Example 2: Specific IP Pair Sampling**
```bash
# Sample HTTP traffic between specific IPs at 10% probability
$ utilities/sample_flow_mod tcp:localhost:6633 \
    -s 10.0.0.1 -d 10.0.0.2 \
    --ip-proto 6 --eth-type 0x0800 \
    --sample-prob 100000 \
    --max-rate 10 \
    --server-ip 192.168.1.100
```

**Example 3: MAC Address Based Sampling**
```bash
# Sample traffic from specific MAC address
$ utilities/sample_flow_mod tcp:localhost:6633 \
    --src-mac 00:11:22:33:44:55 \
    --sample-prob 1000000 \
    --max-rate 1000 \
    --server-ip 192.168.1.100
```

**Example 4: Delete Sampling Rules**
```bash
# Delete all flows matching port 1
$ utilities/sample_flow_mod tcp:localhost:6633 -i 1 -D
```

## 🏗️ Technical Architecture

### Packet Sampling Implementation

The packet sampling feature is implemented through several key components:

#### 1. OpenFlow Protocol Extension
- **New Action Type**: `OFPAT_SAMPLE` (type 28)
- **Standard Compliance**: Uses official OpenFlow action space (no experimenter ID)
- **Rich Parameters**: Probability, rate limiting, server configuration

#### 2. Token Bucket Rate Limiting
- **Per-Flow Rate Control**: Each flow entry has its own token bucket
- **Precise Limiting**: Ensures sampling never exceeds configured maximum rate
- **Burst Handling**: Allows short bursts while maintaining average rate

#### 3. Sampling Pipeline Integration
```
Packet In → Flow Table Match → Sampling Decision → Normal Forwarding
                          ↓
                  Sampled Packet Copy
                          ↓
                  UDP Send to Receiver
```

#### 4. Metadata Enrichment
Each sampled packet includes:
- **Timestamp**: Microsecond precision capture time
- **Input Port**: Original ingress port information
- **Packet Length**: Original packet size
- **Full Packet Data**: Complete Ethernet frame

## 🛠️ Building and Installation

These instructions have been tested on Ubuntu 16.04/18.04/20.04. Other distributions may need different steps.

### Prerequisites

Install required packages:
```bash
$ sudo apt-get update
$ sudo apt-get install cmake libpcap-dev libxerces-c3.1 libxerces-c-dev \
    libpcre3 libpcre3-dev flex bison pkg-config autoconf libtool \
    libboost-dev libpcap0.8-dev
```

### NetBee Library Installation

The switch uses the NetBee library for packet parsing:

1. Clone and build NetBee:
```bash
$ git clone https://github.com/netgroup-polito/netbee.git
$ cd netbee/src
$ cmake .
$ make
```

2. Install NetBee libraries:
```bash
$ sudo cp ../bin/libn*.so /usr/local/lib/
$ sudo ldconfig
$ sudo cp -R ../include/* /usr/include/
```

### Building the Switch

Run the following commands in the `ofsoftswitch13` directory:

```bash
# Generate build system
$ ./boot.sh

# Configure with sampling support
$ ./configure

# Build everything
$ make

# Install (optional)
$ sudo make install
```

### Building the Sampling Tools

The packet sampling tools are built automatically with the main switch:

- `utilities/packet_receiver` - Packet receiver server
- `utilities/sample_flow_mod` - Flow entry management tool

## 🚀 Running the Switch with Sampling

### 1. Start the Datapath

```bash
# Start switch with datapath ID 00:00:00:00:00:01
# Listen on interfaces eth1, eth2
# Open control port 6633
$ sudo udatapath/ofdatapath \
    --datapath-id=0000000000000001 \
    --interfaces=eth1,eth2 \
    ptcp:6633
```

### 2. Connect to Controller (Optional)

```bash
# Connect switch to controller at 192.168.1.1:6633
$ secchan/ofprotocol tcp:localhost:6633 tcp:192.168.1.1:6633
```

### 3. Start Packet Receiver

On your monitoring server (can be same or different machine):

```bash
# Start receiver on port 9999
$ utilities/packet_receiver 9999
```

### 4. Install Sampling Rules

```bash
# Install sampling rule for port 1 traffic
$ utilities/sample_flow_mod tcp:localhost:6633 \
    -i 1 -o 2 \
    --sample-prob 500000 \
    --max-rate 100 \
    --server-ip 192.168.1.100 \
    --server-port 9999
```

## 📊 Monitoring and Analysis

### Real-time Monitoring

The packet receiver provides real-time statistics:
- Packet count and rate
- Source information
- Packet sizes
- Timestamp details

### Offline Analysis

Sampled packets are saved in pcap format, compatible with:
- **Wireshark**: Graphical packet analysis
- **tcpdump**: Command-line packet analysis
- **tshark**: Wireshark's command-line version
- **Custom scripts**: Using libpcap or similar libraries

### Performance Considerations

- **CPU Usage**: Sampling adds minimal overhead (typically < 5%)
- **Network Bandwidth**: Sampled traffic uses separate UDP streams
- **Storage Requirements**: Pcap files can grow large; consider rotation
- **Rate Limiting**: Essential to prevent overload of receiver servers

## 📝 Traditional Configuration

You can still use the traditional `dpctl` utility for basic switch configuration:

```bash
# Check flow statistics
$ utilities/dpctl tcp:localhost:6633 stats-flow table=0

# Install basic flow entry
$ utilities/dpctl tcp:localhost:6633 flow-mod table=0,cmd=add in_port=1 apply:output=2

# Add meter for rate limiting
$ utilities/dpctl tcp:localhost:6633 meter-mod cmd=add,meter=1 drop:rate=50
```

For a complete list of commands and arguments, use the `--help` argument.

## 🤝 Contribute

Please submit your bug reports, fixes and suggestions as pull requests on GitHub, or by contacting us directly.

## 📄 License

OpenFlow 1.3 Software Switch is released under the BSD license (BSD-like for code from the original Stanford switch).

## 🙏 Acknowledgments

This project was supported by Ericsson Innovation Center in Brazil. Formerly maintained by CPqD in technical collaboration with Ericsson Research.

**Packet Sampling Extension** developed to enhance network monitoring capabilities for research and production environments.


## 📞 Contact

E-mail: Xiaodong Dong (dongxiaodong@nankai.edu.cn)

[ofp13]: https://www.opennetworking.org/images/stories/downloads/specification/openflow-spec-v1.3.0.pdf
[ericssonsw11]: https://github.com/TrafficLab/of11softswitch
[compileubuntu14]: http://tocai.dia.uniroma3.it/compunet-wiki/index.php/Installing_and_setting_up_OpenFlow_tools
[beba-eu]: http://www.beba-project.eu/

        
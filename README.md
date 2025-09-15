# e2sim-rc

An O-RAN E2 Node simulator focused on E2AP v2 with the E2SM-RC service model, plus:

- A lightweight O1 REST surface to get/set per-cell TX reference level (gain)
- An Open Fronthaul (OFH) TCP shim that speaks protobuf messages to a simulated RU

The built binary is `e2sim` (see `e2sim/src/e2node.cpp`).

## What’s inside

- E2AP over SCTP with RAN Function registration for E2SM-RC (ID=1)
- E2SM-RC helpers and glue code under `e2sim/e2sm/rc/`
- Shared DU state in `e2sim/common/global_data.*` (thread-safe accessors)
- OFH server in `e2sim/ofh/ofh_du.{hpp,cpp}` using protobuf framing
- O1 HTTP handler in `e2sim/o1/smo_du.{hpp,cpp}` using cpprestsdk

High-level flow:
1) RU connects to OFH TCP server and registers its cells (PCI → socket)
2) O1 can set desired TX gain per PCI; desired values are persisted in DU state
3) A reconciliation loop ensures desired gains are sent/re-sent and verified with timeouts/backoff
4) E2AP runs over SCTP to E2Term and registers E2SM-RC

See `doc/class_diagram.svg` for an overview of interactions.

## Build and run

### Native (Linux)

Prereqs:
- C++20 toolchain, cmake >= 3.16
- libsctp-dev, OpenSSL, cpprestsdk
- Protobuf v29 (runtime and protoc) and prometheus-cpp (static)

Install base packages (Ubuntu example):

```bash
sudo apt-get update
sudo apt-get install -y \
	build-essential git cmake libsctp-dev lksctp-tools \
	autoconf automake libtool bison flex libboost-all-dev \
	libcpprest-dev libssl-dev
```

Fetch submodules and build prometheus-cpp:

```bash
git submodule update --init --recursive
cd e2sim/3rdparty/prometheus-cpp/
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
sudo make install && sudo ldconfig
```

Build the simulator:

```bash
cd ../../..
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Run it:

```bash
./e2sim <E2TERM_IP> -p 36422 -c 001 -n 001 -b 1
```

CLI flags (from `e2sim/src/e2node.cpp`):
- `e2term-address` (positional): E2Term IP or hostname
- `-p, --port` E2Term SCTP port (default 36422)
- `-c, --mcc` gNodeB Mobile Country Code (default 001)
- `-n, --mnc` gNodeB Mobile Network Code (default 001)
- `-b, --nodebid` gNodeB Identity (29-bit, decimal like `15` or hex like `0xF`)

### Docker

This repo includes a multi-stage Dockerfile that builds protobuf v29, prometheus-cpp, and the `e2sim` binary, then produces a slim runtime image.

Build locally:

```bash
docker build . -t e2sim-rc:local
```

Run (expose O1 and OFH ports if you want to use them from the host):

```bash
docker run --rm -it \
	-p 8090:8090 \# O1 REST
	-p 34567:34567 \# OFH TCP
	e2sim-rc:local \
	e2sim <E2TERM_IP> -p 36422 -c 001 -n 001 -b 1
```

Helper script `build.sh` can also tag and push to multiple registries:

```bash
./build.sh -h
./build.sh -r <user/repo:tag> -L <local-registry:port>
```

Kubernetes quick-run helper `build_and_run.sh` expects a local registry on port 5001 and spins up a pod in namespace `ricxapp`.

## Interfaces

### E2AP/E2SM-RC

- Registers E2SM-RC RAN Function with ID=1 on startup
- Encodes/sends E2AP over SCTP to E2Term and handles request/response callbacks inside the E2SM-RC module

### OFH TCP shim

- Listens on TCP port 34567
- Message framing: strictly `[len_be32][OfhMessage bytes]` where `len_be32` is a 4-byte big-endian unsigned length of the protobuf payload. Do not include an inner length or send zero-length frames.
- Protobuf schema at `e2sim/ofh/protos/signaling.proto` (generated files: `signaling.pb.{h,cc}`)
- Multiple cells can be associated to a single socket; the DU tracks cell→socket mapping
- Heartbeat health marks sockets up/down and pauses sends on unhealthy sockets

### O1 REST (tx-gain)

Base path: `http://<du-host>:8090/restconf/operations/tx-gain`

- GET returns current view of cells and their TX reference levels

	Response 200 OK:
	```json
	[
		{"pci": 100, "gain": -5.0},
		{"pci": 101, "gain": -3.5}
	]
	```

- POST sets desired TX reference level for a PCI; the DU persists the desired value and sends an OFH request. On success (or queued for reconciliation) it replies 204 No Content.

	Request:
	```json
	{"pci": 100, "gain": -4.0}
	```

Notes:
- Desired gains are persisted in `GlobalE2NodeData` to reconcile after RU reconnects
- A reconciliation loop with backoff, timeouts, and a small circuit breaker verifies write-after-read

## Development notes

- C++20; logging level set via `LOGGER_LEVEL` in `e2sim/CMakeLists.txt` (defaults to INFO)
- Prefer using `GlobalE2NodeData` accessors rather than globals
- To add new E2SMs, place them under `e2sim/e2sm/<name>/`, add a target like `rc_objects`, add `add_subdirectory(e2sm/<name>)` in `e2sim/CMakeLists.txt`, and register a `RANFunction` in `e2sim/src/e2node.cpp`

## Troubleshooting

- SCTP connection to E2Term: verify IP/port and that SCTP kernel modules are available (lksctp-tools)
- OFH client framing errors: ensure 4-byte big-endian length prefix and no zero-length frames
- O1 listener errors: port conflict on 8090 or invalid JSON payloads

## License

Apache-2.0. See headers in source files for details.


# e2sim-rc

The purpose of this repository is to maintain a local implementation of a simulator for the O-RAN E2 interface. This repository "mirrors" the official source code from the O-RAN SC e2-interface repository with specific focus on the E2SM-RC (E2 Service Model - RAN Control) application.

## Main Functionalities

The simulator implements specific E2SM-RC features supporting closed-loop control and monitoring capabilities of the RAN:

- **E2 Setup & RAN Function Definition**: Registers and exposes the simulator's capabilities and supported functions to the E2 Manager (E2M) upon startup.
- **RIC Subscription (INSERT/REPORT)**: Handles incoming subscription requests from xApps. It accepts actions of type `INSERT` or `REPORT`, spawning threads to generate periodic indication messages. Unsubscription requests also cleanly terminate the ongoing simulation logic for that subscription.
- **RIC Control**: Processes E2SM-RC Control Messages to enforce Slice SLA policies (e.g., allocating specific PRB ratios per PLMN/SST/SD). It validates if capacity boundaries are respected before accepting (`RIC-CONTROL-ACKNOWLEDGE`) or rejecting (`RIC-CONTROL-FAILURE`) the request. 
- **RIC Query**: Handles querying of cell capacity parameters. It returns actual simulated configuration values (DL/UL maximum capacity, total PRBs, and channel bandwidth) to the requesting xApp via a `RIC-QUERY-RESPONSE`.

## Building the Simulator

This repository includes a `Dockerfile` and a `Makefile` to easily build and package the simulator into a Docker container.

By default, the image is tagged as `zanattabruno/e2sim-rc:orion`. You can override the variables during the build process:

```bash
# Build the Docker image
make build

# Build with custom tags
make build REGISTRY=myregistry.com IMAGE_NAME=my-e2sim-rc TAG=v1.0

# Push the Docker image to a registry
make push
```

## Deployment

The simulator can be deployed to a Kubernetes cluster using the provided Helm chart. 

To deploy the application in the `ricxapp` namespace using Helm, simply run:

```bash
make install
```

You can customize the Helm deployment using the following variables:
- `HELM_CHART` (default: `helm/e2sim-helm`)
- `HELM_RELEASE` (default: `e2sim-rc`)
- `HELM_NAMESPACE` (default: `ricxapp`)
- `HELM_VALUES` (default: `helm/e2sim-helm/values.yaml`)
- `HELM_EXTRA_ARGS`

Example custom deployment:
```bash
make install HELM_NAMESPACE=my-namespace HELM_EXTRA_ARGS="--set replicaCount=2"
```

## Helper Scripts

The `scripts/` directory contains CLI wrapper scripts for interacting with various RIC platform components (App Manager, E2 Manager, and Routing Manager) via HTTP requests. 

These are useful for development and operational management in a near-RT RIC environment:

### App Manager CLI (`scripts/appmgr-cli.sh`)
Registers, deregisters, or lists xApps/simulators in the App Manager.

```bash
Usage: ./scripts/appmgr-cli.sh [[-r|--register] [-d|--deregister] {config-file.json}] [-l|--list]
```
- Requires a valid descriptor file (`config-file.json`) containing the `xapp_name` and `version` for registration/deregistration.

### E2 Manager CLI (`scripts/e2mgr-cli.sh`)
Manages the E2 Nodes connected to the E2 Manager.

```bash
Usage: ./scripts/e2mgr-cli.sh [-a|--add] [-l|--list] [-s|--states] [-n|--nodeb inventory_name]
```
- Note: `--add` expects an `input-e2mgr.txt` payload in the directory.

### Routing Manager CLI (`scripts/rtmgr-cli.sh`)
Manages routing configurations in the Routing Manager.

```bash
Usage: ./scripts/rtmgr-cli.sh [-a|--addrmrroute] [-d|--delrmrroute] [-t|--routingtable]
```
- Note: `--addrmrroute` and `--delrmrroute` expect a `routes.json` file in the directory.

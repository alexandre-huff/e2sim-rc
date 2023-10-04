#!/bin/bash

# Requires a local registry attending port 5001.
# docker run -d -p 5001:5000 --name registry registry:2

kubectl delete pod -n ricxapp e2sim &
docker build -t localhost:5001/e2sim .
docker push localhost:5001/e2sim
kubectl run e2sim --image=localhost:5001/e2sim -n ricxapp --restart=Never
echo "To enter the pod, execute kubectl exec -it -n ricxapp e2sim bash"
e2term_ip=$(kubectl get pods -n ricplt -o wide | awk '$1 ~ /e2term/ { print $6 }')
echo "To execute e2sim, inside the pod, execute"
echo "    e2sim-rc ${e2term_ip} -p 36422 -m 724 -c 05 -b 0"

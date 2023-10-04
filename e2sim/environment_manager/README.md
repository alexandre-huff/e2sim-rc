Environment Manager

_Interface for inputing UE data_

This is compiled with e2sim-rc, and provides a json interface to input UE events.

To verify the interface is reachable, execute

~~~
# Get the e2sim ip address
kubectl get pods -A | awk '$1 ~ /e2sim/ { print $6 }'
curl -X GET http://${e2sim_ip}/v1/test
~~~

# vim: ts=4 sw=4 noet
#!/bin/bash

# control.go:	xapp.Resource.InjectRoute("/ric/v1/symptomdata", c.SymptomDataHandler, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/test/{testId}", c.TestRestHandler, "POST")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/restsubscriptions", c.GetAllRestSubscriptions, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/get_all_e2nodes", c.GetAllE2Nodes, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/get_e2node_rest_subscriptions/{ranName}", c.GetAllE2NodeRestSubscriptions, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/get_all_xapps", c.GetAllXapps, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/get_xapp_rest_restsubscriptions/{xappServiceName}", c.GetAllXappRestSubscriptions, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/get_e2subscriptions/{restId}", c.GetE2Subscriptions, "GET")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/delete_all_e2node_subscriptions/{ranName}", c.DeleteAllE2nodeSubscriptions, "DELETE")
# control.go:	xapp.Resource.InjectRoute("/ric/v1/delete_all_xapp_subscriptions/{xappServiceName}", c.DeleteAllXappSubscriptions, "DELETE")

usage() {
	echo "
Usage: $0 [-e|--e2nodes] [-x|--xapps] [-r|--restsub] [-s|--symptomdata]
"
	exit 1
}

if [[ "$#" -eq 0 ]]; then
	usage
fi

SUBMGR_IP=$(kubectl -n ricplt get po -l app=ricplt-submgr -o jsonpath={'.items[0].status.podIP'})
#SUBMGR_PORT=$(kubectl -n ricplt get service service-ricplt-submgr-http -o jsonpath='{.spec.ports[0].nodePort}')
SUBMGR_PORT=8080
SUBMGR_ADDR=${SUBMGR_IP}:${SUBMGR_PORT}

do_get_all_e2nodes() {
	curl -s "http://${SUBMGR_ADDR}/ric/v1/get_all_e2nodes" -H "accept: application/json" | jq .
}

do_get_all_xapps() {
	curl -s "http://${SUBMGR_ADDR}/ric/v1/get_all_xapps" -H "accept: application/json" | jq .
}

do_get_restsub() {
	curl -s "http://${SUBMGR_ADDR}/ric/v1/restsubscriptions" -H "accept: application/json" | jq .
}
do_get_symptomdata() {
	curl -s "http://${SUBMGR_ADDR}/ric/v1/symptomdata" -H "accept: application/json" | jq .
}

while [[ "$#" -gt 0 ]]; do
	case $1 in
		-e|--e2nodes)
			do_get_all_e2nodes
			shift;;
		-x|--xapps)
			do_get_all_xapps
			shift
			;;
		-r|--restsub)
			do_get_restsub
			shift
			;;
		-s|--symptomdata)
			shift
			do_get_symptomdata
			shift
			;;
		*)
			usage
			;;
	esac;
	shift;
done


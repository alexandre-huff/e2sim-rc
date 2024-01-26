#!/bin/bash
helm ls --all --short -n ricxapp | grep bouncer | xargs -L1 helm uninstall -n ricxapp
helm ls --all --short -n ricplt | grep e2sim | xargs -L1 helm uninstall -n ricplt

kubectl rollout restart statefulset statefulset-ricplt-dbaas-server -n ricplt
kubectl rollout restart deployment deployment-ricplt-e2term-alpha -n ricplt
kubectl rollout restart deployment deployment-ricplt-a1mediator -n ricplt
kubectl rollout restart deployment deployment-ricplt-alarmmanager -n ricplt
kubectl rollout restart deployment deployment-ricplt-appmgr -n ricplt
kubectl rollout restart deployment deployment-ricplt-e2mgr -n ricplt
kubectl rollout restart deployment deployment-ricplt-o1mediator -n ricplt
kubectl rollout restart deployment deployment-ricplt-rtmgr -n ricplt
kubectl rollout restart deployment deployment-ricplt-submgr -n ricplt
kubectl rollout restart deployment deployment-ricplt-vespamgr -n ricplt
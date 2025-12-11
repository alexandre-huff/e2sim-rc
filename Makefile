# Variables (override at runtime: make REGISTRY=example.com TAG=v1)
REGISTRY ?= docker.io
IMAGE_NAME ?= zanattabruno/e2sim-rc
TAG ?= orion

IMAGE_URI := $(REGISTRY)/$(IMAGE_NAME):$(TAG)

# Helm install knobs
HELM_CHART ?= helm/e2sim-helm
HELM_RELEASE ?= e2sim-rc
HELM_NAMESPACE ?= ricxapp
HELM_VALUES ?= $(HELM_CHART)/values.yaml
HELM_EXTRA_ARGS ?=

.PHONY: build push install

build:
	docker build -t $(IMAGE_URI) .

push: build
	docker push $(IMAGE_URI)

install:
	helm upgrade --install $(HELM_RELEASE) $(HELM_CHART) \
	  --namespace $(HELM_NAMESPACE) --create-namespace \
	  -f $(HELM_VALUES) $(HELM_EXTRA_ARGS)

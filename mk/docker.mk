# Docker-based build targets
# Usage: make docker
#        make in-docker stm32-mock

DOCKER_IMAGE_NAME = embys-fw-dev
DOCKER_TAG ?= latest
PROJECT_ROOT ?= $(shell pwd)
WORK_DIR ?= /work

# Detect if we're running inside Docker container
# Check for /.dockerenv file (created by Docker) or DOCKER_CONTAINER environment variable
INSIDE_DOCKER := $(shell test -f /.dockerenv && echo "true" || echo "false")
ifeq ($(DOCKER_CONTAINER),true)
    INSIDE_DOCKER := true
endif

.docker: $(PROJECT_ROOT)/Dockerfile
	@echo "Building Docker image: $(DOCKER_IMAGE_NAME):$(DOCKER_TAG)"
	cd $(PROJECT_ROOT) && \
	docker build -t $(DOCKER_IMAGE_NAME):$(DOCKER_TAG) . && \
	touch $@

# Build Docker container from Dockerfile
docker: $(PROJECT_ROOT)/.docker

# Run make $(TARGET) inside Docker container
# Mounts project root to /work directory in container
in-docker: $(PROJECT_ROOT)/.docker
	@echo "Running 'make $(TARGET)' in Docker container"
	@echo "Project root: $(PROJECT_ROOT)"
	cd $(PROJECT_ROOT) && \
	docker run --rm -it \
		-v $(PROJECT_ROOT):/work \
		-w $(WORK_DIR) \
		$(DOCKER_IMAGE_NAME):$(DOCKER_TAG) \
		make $(TARGET)

%-in-docker: $(PROJECT_ROOT)/.docker
	@echo "Running 'make $*' in Docker container"
	@echo "Project root: $(PROJECT_ROOT)"
	cd $(PROJECT_ROOT) && \
	docker run --rm -it \
		-v $(PROJECT_ROOT):/work \
		-w $(WORK_DIR) \
		$(DOCKER_IMAGE_NAME):$(DOCKER_TAG) \
		make TC=$(TC) $*

docker-shell:
	@echo "Starting interactive shell in Docker container"
	cd $(PROJECT_ROOT) && \
	docker run --rm -it \
		-v $(PROJECT_ROOT):/work \
		-w $(WORK_DIR) \
		$(DOCKER_IMAGE_NAME):$(DOCKER_TAG) \
		/bin/bash

docker-cmd:
	@echo "Running command in Docker container: $(CMD)"
	cd $(PROJECT_ROOT) && \
	docker run --rm \
		-v $(PROJECT_ROOT):/work \
		-w $(WORK_DIR) \
		$(DOCKER_IMAGE_NAME):$(DOCKER_TAG) \
		bash -c '$(CMD)'

# Clean up Docker images
docker-clean:
	@echo "Removing Docker image: $(DOCKER_IMAGE_NAME):$(DOCKER_TAG)"
	cd $(PROJECT_ROOT) && \
	docker rmi $(DOCKER_IMAGE_NAME):$(DOCKER_TAG) || true

# Show Docker-related information
docker-info:
	@echo "Docker image name: $(DOCKER_IMAGE_NAME):$(DOCKER_TAG)"
	@echo "Project root: $(PROJECT_ROOT)"
	@echo "Available targets:"
	@echo "  docker        - Build Docker container"
	@echo "  in-docker     - Run make \$(TARGET) in Docker"
	@echo "  docker-shell  - Interactive shell in Docker"
	@echo "  docker-clean  - Remove Docker image"

.PHONY: docker in-docker docker-shell docker-clean docker-info

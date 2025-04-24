DOCKER_BASE_DIR=$(DAM_GTM_DIR)
include $(DOCKER_BASE_DIR)/settings.sh

.PHONY: start-docker-container stop-docker-container log-into-container build-docker
build-docker: .build-docker
.build-docker: $(DOCKER_BASE_DIR)/docker/Dockerfile
	docker build -t $(IMAGE_NAME) --build-arg UID=`id --user` $(DOCKER_BASE_DIR)/docker
	docker image inspect --format='{{.Id}}' $(IMAGE_NAME) > $@
	
start-docker-container: .build-docker
	DOCKER_BASE_DIR=$(DOCKER_BASE_DIR) bash $(DOCKER_BASE_DIR)/docker_setup.sh $@

stop-docker-container:
	DOCKER_BASE_DIR=$(DOCKER_BASE_DIR) bash $(DOCKER_BASE_DIR)/docker_setup.sh $@

log-into-container:
	DOCKER_BASE_DIR=$(DOCKER_BASE_DIR) bash $(DOCKER_BASE_DIR)/docker_setup.sh $@


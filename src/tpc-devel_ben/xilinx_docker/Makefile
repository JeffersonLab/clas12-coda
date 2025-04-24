include settings.sh

build-docker: .build-docker
.build-docker: docker/Dockerfile
	docker build -t $(IMAGE_NAME) --build-arg UID=`id --user` docker
	docker image inspect --format='{{.Id}}' $(IMAGE_NAME) > $@
	
start-docker-container: .build-docker
	bash docker_setup.sh $@

stop-docker-container:
	docker kill $(CONTAINER_NAME) 

log-into-container:
	bash docker_setup.sh $@

.SUFFIXES:

PREPARE ?= 0

#IMAGE_centos-7 = registry.centos.org/centos/centos:7

PRE_alpine-latest = RUN apk add --no-cache bash\nENV NO_SYSTEMD=1

# 1 = name
# 2 = message
define PRINT
	@printf '%s\n' '' '$(strip $1) $(strip $2)' ''
endef

# 1 = name
# 2 = Dockerfile
# 3 = config
define DOCKER_BUILD
	$(call PRINT, $1, $3)
	docker build -f $(strip $2) --build-arg CONFIG=$(strip $3) \
		--build-arg PREPARE=$(PREPARE) --force-rm $(DOCKER_ARGS) ../..
endef

.PHONY: all
all: main ubuntu-focal centos-8 ubuntu-bionic ubuntu-xenial centos-7
# ubuntu-bionic debian-stretch

.PHONY: centos-%
centos-%: Dockerfile.centos-%
	$(call DOCKER_BUILD, $@, $<, minimal)
	$(call DOCKER_BUILD, $@, $<, full)
	$(call DOCKER_BUILD, $@, $<, rpm)
	$(call PRINT, $@, done)

.PHONY: ubuntu-%
ubuntu-%: Dockerfile.ubuntu-%
	$(call DOCKER_BUILD, $@, $<, minimal)
	$(call DOCKER_BUILD, $@, $<, full)
	$(call DOCKER_BUILD, $@, $<, deb)
	$(call PRINT, $@, done)

.PHONY: debian-%
debian-%: Dockerfile.debian-%
	$(call DOCKER_BUILD, $@, $<, minimal)
	$(call DOCKER_BUILD, $@, $<, full)
	$(call DOCKER_BUILD, $@, $<, deb)
	$(call PRINT, $@, done)

.PHONY: alpine-%
alpine-%: Dockerfile.alpine-%
	$(call DOCKER_BUILD, $@, $<, minimal)
	$(call DOCKER_BUILD, $@, $<, full)
	$(call PRINT, $@, done)

.PHONY: main
main:
	$(call PRINT, $@, build)
	cd ../.. && docker build -f Dockerfile --build-arg BUILD=Release .
	$(call PRINT, $@, done)

.SECONDARY: Dockerfile.%
Dockerfile.%: BASE_IMAGE = $(or $(IMAGE_$*),$(subst -,:,$*))
Dockerfile.%: Dockerfile
	sed -re 's/base:image/$(subst /,\/,$(BASE_IMAGE))/' \
		-e 's/^#PRE/$(subst /,\/,$(PRE_$*))/' \
		$< > $@

.PHONY: clean
clean:
	rm -f Dockerfile.*

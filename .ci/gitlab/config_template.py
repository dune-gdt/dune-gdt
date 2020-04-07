#!/usr/bin/env python3
# kate: indent-width 2;

tpl = '''# THIS FILE IS AUTOGENERATED -- DO NOT EDIT       #
# Edit and Re-run .ci/gitlab/config_tpl.py instead  #
stages:
  - test

variables:
    GIT_SUBMODULE_STRATEGY: recursive
    TRAVIS_BRANCH: ${CI_COMMIT_REF_NAME}
    TRAVIS_COMMIT: ${CI_COMMIT_SHA}
    MY_MODULE: dune-xt

.subdir-test:
    tags:
      - docker-in-docker
      - long execution time
    stage: test
    retry:
        max: 2
        when:
            - runner_system_failure
            - stuck_or_timeout_failure
            - api_failure
    only: ['branches', 'tags', 'triggers', 'merge_requests']
    except:
        - /^staging/.*$/i
    retry:
        max: 2
        when:
            - always
    image: docker:stable
    variables:
        DOCKER_HOST: tcp://docker:2375/
        DOCKER_DRIVER: overlay2
        IMAGE: $CI_REGISTRY/ag-ohlberger/dune-community/dune-xt/ci_testing_${DOCKER_TAG}:${CI_COMMIT_SHORT_SHA}
    before_script:
      - |
        apk --update add openssh-client rsync git file bash python3 curl
        pip3 install -U docker jinja2 docopt

        export BASEIMAGE="${MY_MODULE}-testing_${DOCKER_TAG}:${CI_COMMIT_REF_NAME/\//_}"
        # get image with fallback to master branch of the super repo
        docker pull dunecommunity/${BASEIMAGE} || export BASEIMAGE="${MY_MODULE}-testing_${DOCKER_TAG}:master" ; docker pull dunecommunity/${BASEIMAGE}
        export ENV_FILE=${HOME}/env
        mkdir ${CI_PROJECT_DIR}/testresults && chmod -R 777 ${CI_PROJECT_DIR}/testresults
        DOCKER_RUN="docker run -v ${CI_PROJECT_DIR}/testresults:/home/dune-ci/testresults --env-file ${ENV_FILE} ${IMAGE}"
        git submodule update --init --recursive
        docker build --build-arg BASE=${BASEIMAGE} -t ${IMAGE} -f .ci/shared/docker/ci_run/Dockerfile .
    script:
      - |
        [[ -f ./.gitsuper ]] && echo "Please remove .gitsuper from the repo" && exit 1
        python3 ./.ci/shared/scripts/make_env_file.py
        ${DOCKER_RUN} /home/dune-ci/src/${MY_MODULE}/.ci/shared/scripts/test_cpp.bash
        ${DOCKER_RUN} /home/dune-ci/src/${MY_MODULE}/.ci/shared/scripts/test_python.bash
        touch $CI_PROJECT_DIR/success

    after_script:
      - |
        if [ -e $CI_PROJECT_DIR/success ]; then
            echo "Not pushing ${IMAGE}"
        else
            docker login -u $CI_REGISTRY_USER -p $CI_REGISTRY_PASSWORD $CI_REGISTRY
            docker push ${IMAGE}
        fi

    artifacts:
      reports:
        junit: '${CI_PROJECT_DIR}/testresults/*xml'
    services:
        - docker:dind
    environment:
        name: unsafe

{% for image, subdir in matrix %}
{{subdir}} {{ image[image.find('debian')+1+6:] }}:
    extends: .subdir-test
    variables:
        DOCKER_TAG: {{ image }}
        TESTS_MODULE_SUBDIR: {{ subdir }}
    {%- if subdir in ['functions', 'la'] and 'gcc' in image %}
    tags:
        - amm-only
    {%- endif %}
{% endfor %}

'''

import os
import jinja2
import sys
from itertools import product
tpl = jinja2.Template(tpl)
images = ['debian-unstable_gcc_full', 'debian_gcc_full', 'debian_clang_full']
subdirs = ['common', 'grid', 'functions', 'la']
matrix = product(images, subdirs)
with open(os.path.join(os.path.dirname(__file__), 'config.yml'), 'wt') as yml:
    yml.write(tpl.render(matrix=matrix))

#!/usr/bin/env python3
# kate: indent-width 2;

tpl = '''# THIS FILE IS AUTOGENERATED -- DO NOT EDIT       #
# Edit and Re-run .ci/gitlab/config_template.py instead  #
stages:
  - images
{%- for kind in kinds %}
  - {{kind}}
{%- endfor %}
  - python

variables:
    GIT_SUBMODULE_STRATEGY: recursive
    TRAVIS_BRANCH: ${CI_COMMIT_REF_NAME}
    TRAVIS_COMMIT: ${CI_COMMIT_SHA}
    MY_MODULE: dune-xt
    CCACHE_BASEDIR: ${CI_PROJECT_DIR}
    CCACHE_DIR: "${CI_PROJECT_DIR}/.ccache"
    CCACHE_COMPILERCHECK: content
    CCACHE_COMPRESS: "true"
    PYTEST_ADDOPTS: "-s"
    BASE_PROJECT: zivgitlab.wwu.io/ag-ohlberger/dune-community/dune-xt-super

.image_builder:
    tags:
      - docker-in-docker
      - long execution time
    stage: images
    retry:
        max: 2
        when:
            - runner_system_failure
            - stuck_or_timeout_failure
            - api_failure
    rules:
        - if: '$CI_COMMIT_REF_NAME =~ /^staging.*/'
          when: never
        - when: on_success
    retry:
        max: 2
        when:
            - always
    image: harbor.uni-muenster.de/proxy-docker/library/docker:19.03.12
    variables:
        DOCKER_HOST: tcp://docker:2375/
        DOCKER_DRIVER: overlay2
        IMAGE: ${CI_REGISTRY_IMAGE}/ci_testing_${CI_IMAGE}:${CI_COMMIT_SHORT_SHA}
    before_script:
      - |
        docker login -u $CI_REGISTRY_USER -p $CI_REGISTRY_PASSWORD $CI_REGISTRY
        apk --update add py3-pip openssh-client rsync git file bash python3 curl
        pip3 install -U docker jinja2 docopt

        export BASEIMAGE="${MY_MODULE}-testing_${CI_IMAGE}:${CI_COMMIT_REF_NAME/\//_}"
        # get image with fallback to master branch of the super repo
        docker pull ${BASE_PROJECT}/${BASEIMAGE} || export BASEIMAGE="${MY_MODULE}-testing_${CI_IMAGE}:master" ; docker pull ${BASE_PROJECT}/${BASEIMAGE}
    script:
      - |
        git submodule update --init --recursive
        docker build --build-arg BASE=${BASEIMAGE} -t ${IMAGE} -f .ci/gitlab/Dockerfile .
        docker push ${IMAGE}
    services:
        - name: harbor.uni-muenster.de/proxy-docker/library/docker:dind
          alias: docker
    environment:
        name: unsafe

.subdir-test:
    tags:
      - long execution time
    stage: test
    retry:
        max: 2
        when:
            - runner_system_failure
            - stuck_or_timeout_failure
            - api_failure
    rules:
        - if: '$CI_COMMIT_REF_NAME =~ /^staging.*/'
          when: never
        - when: on_success
    retry:
        max: 2
        when:
            - always
    image: ${CI_REGISTRY_IMAGE}/ci_testing_${CI_IMAGE}:${CI_COMMIT_SHORT_SHA}
    cache:
      key: "$CI_JOB_NAME"
      paths:
        - .ccache
    before_script:
      - |
        mkdir /home/dune-ci/testresults && chmod -R 777 /home/dune-ci/testresults
        [[ -f ./.gitsuper ]] && echo "Please remove .gitsuper from the repo" && exit 1
        ccache --zero-stats || true
    after_script:
      - ccache --show-stats
    artifacts:
      reports:
        junit: '/home/dune-ci/testresults/*xml'
    environment:
        name: unsafe

{% for image in images -%}
{{ image }}:
    extends: .image_builder
    variables:
        CI_IMAGE: {{ image }}
{% endfor %}

{% for image, subdir, kind in matrix %}
{{subdir}} {{ image[image.find('debian')+1+6:] }} {{kind}}:
    extends: .subdir-test
    variables:
        CI_IMAGE: {{ image }}
        TESTS_MODULE_SUBDIR: {{ subdir }}
    {%- if 'gcc' in image %}
    tags:
        - dustin
    {%- endif %}
    stage: {{kind}}
    needs: ["{{image}}"]
    script:
          - /home/dune-ci/src/${MY_MODULE}/.ci/shared/scripts/test_{{kind}}.bash
{% endfor %}

{% for image in images %}
{{ image[image.find('debian')+1+6:] }} python:
    extends: .subdir-test
    variables:
        CI_IMAGE: {{ image }}
    {%- if 'gcc' in image %}
    tags:
        - dustin
    {%- endif %}
    stage: python
    needs: ["{{image}}"]
    script:
          - /home/dune-ci/src/${MY_MODULE}/.ci/shared/scripts/test_python.bash
{% endfor %}

'''

import os
import jinja2
import sys
from itertools import product
tpl = jinja2.Template(tpl)
images = ['debian-unstable_gcc_full', 'debian_gcc_full', 'debian_clang_full']
subdirs = ['common', 'grid', 'functions', 'la']
kinds = ['cpp', 'headercheck']
matrix = product(images, subdirs, kinds)
with open(os.path.join(os.path.dirname(__file__), 'config.yml'), 'wt') as yml:
    yml.write(tpl.render(matrix=matrix, images=images, kinds=kinds))

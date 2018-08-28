# Integrating docker-based test servers into Qt Test framework
#
# This file adds support for docker-based test servers built by testcase
# projects that need them. To enable this feature, any automated test can
# include testserver.pri in its project file. This instructs qmake to insert
# additional targets into the generated Makefile. The 'check' target then brings
# up test servers before running the testcase, and shuts them down afterwards.
#
# TESTSERVER_COMPOSE_FILE
# - Contains the path of docker-compose file
# This configuration file defines the services used for autotests. It tells the
# docker engine how to build up the docker images and containers. In qtbase, a
# shared docker-compose file is located in the tests folder.
# Example: TESTSERVER_COMPOSE_FILE = \
#              $$dirname(_QMAKE_CONF_)/tests/testserver/docker-compose.yml
#
# The user must run the provisioning scripts in advance before attempting to
# build the test servers. The docker_testserver.sh script is used to build up
# the docker images into the docker-cache. It handles the immutable parts of the
# server installation that rarely need adjustment, such as downloading packages.
# Example: qt5/coin/provisioning/.../testserver/docker_testserver.sh
#
# QT_TEST_SERVER_LIST
# - A list of test servers to bring up for this testcase
# These test servers should be defined in $$TESTSERVER_COMPOSE_FILE. Each
# testcase can define the test servers it depends on.
# Example: QT_TEST_SERVER_LIST = apache2 squid vsftpd ftp-proxy danted
#
# Pre-processor defines needed for the application:
# QT_TEST_SERVER
# - A preprocessor macro used for testcase to change testing parameters at
#   compile time
# This macro is predefined for docker-based test servers and is passed as a
# compiler option (-DQT_TEST_SERVER). The testcase can then check whether
# docker-based servers are in use and change the testing parameters, such as
# host name or port number, at compile time. An example can be found in
# network-settings.h.
#
# Example:
# #if defined(QT_TEST_SERVER)
#     Change the testing parameters at compile time
# #endif
#
# QT_TEST_SERVER_DOMAIN
# - A preprocessor macro that holds the server domain name
# Provided for the helper functions in network-settings.h. Use function
# serverDomainName() in your application instead.
#
# Additional make targets:
# 1. check_network - A renamed target from the check target of testcase feature.
# 2. testserver_clean - Clean up server containers/images and tidy away related
#    files.

TESTSERVER_COMPOSE_FILE = $$dirname(_QMAKE_CONF_)/tests/testserver/docker-compose.yml
TESTSERVER_VERSION = $$system(docker-compose --version)

equals(QMAKE_HOST.os, Windows)|isEmpty(TESTSERVER_VERSION) {
    # Make check with server "qt-test-server.qt-test-net" as a fallback
    message("testserver: qt-test-server.qt-test-net")
} else {
    # Make check with test servers
    message("testserver:" $$TESTSERVER_VERSION)

    # Ensure that the docker-compose file is provided. It is a configuration
    # file which is mandatory for all docker-compose commands. You can get more
    # detail from the description of TESTSERVER_COMPOSE_FILE above. There is
    # also an example showing how to configure it manually.
    FILE_PRETEST_MSG = "Project variable 'TESTSERVER_COMPOSE_FILE' is not set"
    testserver_pretest.commands = $(if $$TESTSERVER_COMPOSE_FILE,,$(error $$FILE_PRETEST_MSG))

    # Before starting the test servers, it requires the user to run the setup
    # script (coin/provisioning/.../testserver/docker_testserver.sh) in advance.
    IMAGE_PRETEST_CMD = docker images -aq "qt-test-server-*"
    IMAGE_PRETEST_MSG = "Docker image qt-test-server-* not found"
    testserver_pretest.commands += $(if $(shell $$IMAGE_PRETEST_CMD),,$(error $$IMAGE_PRETEST_MSG))

    # The domain name is relevant to https keycert (qnetworkreply/crts/qt-test-net-cacert.pem).
    DNSDOMAIN = test-net.qt.local
    TEST_ENV += TESTSERVER_DOMAIN=$$DNSDOMAIN
    DEFINES += QT_TEST_SERVER QT_TEST_SERVER_DOMAIN=$$shell_quote(\"$${DNSDOMAIN}\")

    # There is no docker bridge on macOS. It is impossible to ping a container.
    # Docker docs recommends using port mapping to connect to a container.
    equals(QMAKE_HOST.os, Darwin): TEST_ENV += TESTSERVER_BIND_LOCAL=1

    # Rename the check target of testcase feature
    check.target = check_network
    testserver_test.target = check

    # Pretesting test servers environment
    testserver_test.depends = testserver_pretest

    # Bring up test servers and make sure the services are ready.
    testserver_test.commands = $$TEST_ENV docker-compose -f $$TESTSERVER_COMPOSE_FILE up -d \
                               --force-recreate --timeout 1 $${QT_TEST_SERVER_LIST} &&

    # Check test cases with docker-based test servers.
    testserver_test.commands += $(MAKE) check_network;

    # Stop and remove test servers after testing.
    testserver_test.commands += $$TEST_ENV docker-compose -f $$TESTSERVER_COMPOSE_FILE down \
                                --timeout 1

    # Destroy test servers and tidy away related files.
    testserver_clean.commands = $$TEST_ENV docker-compose -f $$TESTSERVER_COMPOSE_FILE down \
                                --rmi all

    QMAKE_EXTRA_TARGETS += testserver_pretest testserver_test testserver_clean
}

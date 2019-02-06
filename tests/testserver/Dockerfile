# This Dockerfile is used on windows as volume sharing / mounting does not work in conjunction
# docker-machine. Windows Update 1809 might solve some of those issues when using docker0
# network switch, but nothing has been reported in regards to the combination with docker-machine.

ARG provisioningImage
FROM $provisioningImage

# Common is used for all test images so far, no need for a variable
COPY ./common /common
ARG servicedir
COPY $servicedir /service

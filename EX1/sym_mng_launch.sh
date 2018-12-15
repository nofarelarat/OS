#!/bin/bash

fullPath="${PATH_TO_DATA}/${DATA_FILE}"
chmod 700 $fullPath
${FULL_EXE_NAME} $fullPath ${PATTERN} ${BOUND} &


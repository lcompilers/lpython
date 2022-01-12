#!/usr/bin/env bash

set -ex

if [[ "$1" == "running" ]]; then
    status="running"
else
    if [[ "${AGENT_JOBSTATUS}" == "Succeeded" ]]; then
        status="success"
    else
        status="failed"
    fi
fi

project_id=4494718 # lfortran/lfortran

curl --request POST --header "PRIVATE-TOKEN: ${TOKEN_STATUS}" "https://gitlab.com/api/v4/projects/${project_id}/statuses/${COMMIT_ID}?state=${status}&name=AzurePipelines&target_url=https%3A%2F%2Fdev.azure.com%2Flfortran%2Flfortran%2F_build%2Fresults%3FbuildId%3D${BUILD_ID}"

project_id=8213176 # certik/lfortran

curl --request POST --header "PRIVATE-TOKEN: ${TOKEN_STATUS}" "https://gitlab.com/api/v4/projects/${project_id}/statuses/${COMMIT_ID}?state=${status}&name=AzurePipelines&target_url=https%3A%2F%2Fdev.azure.com%2Flfortran%2Flfortran%2F_build%2Fresults%3FbuildId%3D${BUILD_ID}"

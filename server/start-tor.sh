#!/bin/bash

run_cmd() {
    echo -e "\033[35mexecuting: $1\033[0m"
    $1
}


run_cmd "sudo systemctl stop tor"
run_cmd "sudo systemctl start tor"
run_cmd "systemctl status tor"
run_cmd "sudo cat /var/lib/tor/hidden_service/hostname"


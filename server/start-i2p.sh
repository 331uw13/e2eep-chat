#!/bin/bash


run_cmd() {
    echo -e "\033[34m-> $1\033[0m"
    $1
}


run_cmd "sudo systemctl start i2pd"
run_cmd "systemctl status i2pd"



#!/bin/bash

#sh make.sh -DUSE_AHEAD_TYPE=1

#ls --file-type |egrep -v "[\./]" |xargs 
targets="base_stress_test base_test empty_test malloc_test mapping_test zbitset_test zbuddy_test zclock_test zforeach_adv_test zforeach_base_test zforeach_mempool_test zhash_map_test zmem_pool_test zshm_boot_test zshm_loader_test zshm_ptr_test zsingle_test zstream_test zsymbols_test"



for target in $targets;
do
    echo "begin do $target"
    ./$target 

    if [ $? -ne 0 ]; then
        exit $?
    fi
    echo ""
    echo ""
    echo ""
    echo ""
done



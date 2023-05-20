

#include "fn_log.h"
#include <string>
#include "zarray.h"
#include "zvector.h"
#include "zlist.h"
#include "zlist_ext.h"
#include "zhash_map.h"
#include "zcontain_test.h"
#include "zbitset.h"

s32 zbitset_test()
{
    u64 array_data[100];
    zbitset bs;
    bs.attach(array_data, 100, true);
    bs.set_with_win(1);
    bs.set_with_win(3);
    bs.set_with_win(700);
    bs.set_with_win(6399);
    
    u32 bit_id = bs.first_bit();
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 1);
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 3);
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 700);
    ASSERT_TEST((bit_id = bs.pick_next_with_win(bit_id)) == 6399);



    return 0;
}


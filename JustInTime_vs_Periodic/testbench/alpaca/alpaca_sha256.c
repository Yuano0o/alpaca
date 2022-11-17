#include <testbench/alpaca.h>
#include <testbench/global_declaration.h>
#include <testbench/testbench_api.h>
//todo 
//find war 
//pre_commit 

typedef uint32_t WORD;

//__GLOBAL_SCALAR or ARRAY
static __nv WORD msg[16];  // message block for 512-bit.

static __nv uint16_t ei, ri;
static __nv WORD msg_exp[64];       // expanded message blocks.
static __nv WORD msg_hash[8];       // hash results.
static __nv WORD round_regs[64][8]; // round registers.


static __nv uint16_t  status = 0; 


void alpaca_sha256_main()
{

    switch(__GET_CURTASK) {
        case 0: goto vector_init;
        case 1: goto message_padding;
        case 2: goto message_expansion;
        case 3: goto message_round_init;
        case 4: goto message_round;
        case 5: goto message_round_final;
        }

__TASK(0,vector_init);
    for (uint16_t i = 0; i < 8; ++i){
        __GET(msg_hash[i]) = INIT_VEC[i];
    }

__TASK(1,message_padding);
    // Set message to all 0-bits.
    memset(__GET(msg), 0, sizeof(WORD) * 16);      

    // Copy HASH_MSG to the beginning of message.
    uint16_t msg_len = strlen(HASH_MSG);
    for (uint16_t i = 0; i < msg_len; ++i){
        ((uint8_t*)__GET(msg))[i] = HASH_MSG[i];
    }

    // Pad 1 in the following bit.
    uint16_t word_index = msg_len / 4;
    uint16_t bit_index = 8 * (msg_len % 4);
    BIT_SET(__GET(msg[word_index]), bit_index);
    // The 0 is automatically padded at first.
    // Pad len of the message.
    __GET(msg[14]) = 0;
    __GET(msg[15]) = (WORD)(8 * msg_len);

    __GET(ei) = 0;
    __TRANSITION_TO(2,message_expansion);


__TASK(2,message_expansion);
    if (__GET(ei) < 16) {
        __GET(msg_exp[__GET(ei)]) = __GET(msg[__GET(ei)]);
    } else {
        __GET(msg_exp[__GET(ei)]) = SIG1(__GET(msg_exp[__GET(ei) - 2]))
                                    + __GET(msg_exp[__GET(ei) - 7]) 
                                    + SIG0(__GET(msg_exp[__GET(ei) - 15]))
                                    + SIG0(__GET(msg_exp[__GET(ei) - 16]));
    }

    __GET(ei)++;
    if (__GET(ei) >= 64){
        __TRANSITION_TO(4, message_round);
    }
    else
    {
        __TRANSITION_TO(3,message_round_init);
    }

__TASK(3,message_round_init);
    for (uint16_t i = 0; i < 8; ++i) {
        __GET(round_regs[0][i]) = __GET(msg_hash[i]);
    }
    __GET(ri) = 0;
    __TRANSITION_TO(4,message_round);


__TASK(4,message_round);

    WORD t1 = __GET(round_regs[__GET(ri)][7]) 
                + BIGSIG1(__GET(round_regs[__GET(ri)][4]))
                + CH(__GET(round_regs[__GET(ri)][4]), 
                        __GET(round_regs[__GET(ri)][5]), 
                        __GET(round_regs[__GET(ri)][6])) 
                + RCST[__GET(ri)] 
                + __GET(msg_exp[__GET(ri)]);

    WORD t2 = BIGSIG0(__GET(round_regs[__GET(ri)][0])) 
                + MA(__GET(round_regs[__GET(ri)][0]), 
                        __GET(round_regs[__GET(ri)][1]), 
                        __GET(round_regs[__GET(ri)][2]));

    __GET(round_regs[__GET(ri)][7]) = __GET(round_regs[__GET(ri)][6]);
    __GET(round_regs[__GET(ri)][6]) = __GET(round_regs[__GET(ri)][5]);
    __GET(round_regs[__GET(ri)][5]) = __GET(round_regs[__GET(ri)][4]);
    __GET(round_regs[__GET(ri)][4]) = __GET(round_regs[__GET(ri)][3]) + t1;
    __GET(round_regs[__GET(ri)][3]) = __GET(round_regs[__GET(ri)][2]);
    __GET(round_regs[__GET(ri)][2]) = __GET(round_regs[__GET(ri)][1]);
    __GET(round_regs[__GET(ri)][1]) = __GET(round_regs[__GET(ri)][0]);
    __GET(round_regs[__GET(ri)][0]) = t1 + t2;

    __GET(ri)++;
    if (__GET(ri) >= 64) {
        __TRANSITION_TO(5,message_round_final);
    }
    else {
        __TRANSITION_TO(4,message_round);
    }

__TASK(5,message_round_final);
    for (uint16_t i = 0; i < 8; ++i) {
        __GET(msg_hash[i]) = __GET(round_regs[63][i]);
    }
    __TASK_DOWN;

}



#include <Kernel/ink.h>
#include <Kernel/scheduler/thread.h>
#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "./rsa.h"
#include "msp-math.h"


const uint32_t may_war_set_rsa[NUM_TEB_RSA][2];
const uint8_t teb_breaking_rsa[NUM_TEB_RSA];

#define MSG "hello"
#define MSG_LEN 5

// Debug defines and flags.
#define DEBUG_PORT 3
#define DEBUG_PIN  4
__nv uint8_t full_run_started = 0;

char * msgPt = MSG;

/**
 * 1. TEB declaration here.
 */
TEB(initTask);      //0   (32bit)
TEB(ce_1);          //1     8,1??  i
TEB(ce_2);          //2     
TEB(is_i_prime);    //3
TEB(ce_3);          //4
TEB(cd);            //5
TEB(ce_4);          //6     4,1   k
TEB(encrypt_init);  //7
TEB(encrypt_inner_loop);//8     52 3
TEB(encrypt_finish);    //9     53 1
TEB(encrypt_print);     //10
TEB(decrypt_init);     encrypt_inner_loop //11
TEB(decrypt_inner_loop);//12        58 3
TEB(decrypt_finish);    //13        59 1
TEB(decrypt_print);     //14

/**
 * 2. Shared variable declaration here.
 */
__shared(
#if (OUR2==0)   //1~25
    long int p;         //1
    long int q;
    long int n;
    long int t;         //4
    long int k;
    long int j; //8
    long int i;         //
    long int flag;      //
    long int e[10];     //18
    long int d[10];     //10  
    long int m[10];     //38
    long int temp[10];  //48
    long int en[10];    //13  
    long int en_pt;
    long int en_ct;
    long int en_key;    //52
    long int en_k;  //53
    long int en_cnt;
    long int en_j;      
    long int de_pt;
    long int de_ct;
    long int de_key;    //58
    long int de_k;  //59
    long int de_cnt;
    long int de_j;      
#else       //1-2-3-6-9-10-12-13-15-20-21-5-4-8-7-14-11-16-19-17-18-22-23-24-25
    long int p;
    long int q;
    long int n;
    long int j;
    long int e[10];
    long int d[10];
    long int temp[10];
    long int en[10];
    long int en_ct;
    long int de_pt;
    long int de_ct;
    long int k;
    long int t;         //4
    long int flag;
    long int i;
    long int en_pt;
    long int m[10];
    long int en_key;
    long int en_j;
    long int en_k;
    long int en_cnt;
    long int de_k;
    long int de_cnt;
    long int de_j;
    long int de_key;    //22
#endif
)

// scalar: declaration of the buffer 
__nv long int i_priv;
__nv long int k_priv;

__nv long int en_k_priv;
__nv long int en_cnt_priv;
__nv long int en_j_priv;

__nv long int de_k_priv
__nv long int de_cnt_priv;
__nv long int de_j_priv;


/**
 * 3. TEB definition here.
 */
TEB(initTask)//0
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    full_run_started = 1;

#ifdef DEBUG
    uart_sendText("Start\n\r", 7);
    uart_sendText("\n\r", 2);
#endif

    int in_p = 7;
    int in_q = 17;
    int ii = 0;

    __SET(p)= in_p;
    __SET(q)= in_q;
    __SET(n)= in_p * in_q;
    __SET(t)= (in_p-1) * (in_q-1);
    __SET(i)=1;
    __SET(k)=0;
    __SET(flag)=0;
    for (ii = 0; ii < MSG_LEN; ii++)
    {
        __SET(m[ii]) = *(msgPt+ii);
    }

#ifdef TSK_SIZ
    cp_sendRes ("initTask \0");
#endif

    NEXT(1);
}

TEB(ce_1)//1
//war i 
i_priv = __GET(i);
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    __SET(i_priv)++; // start with i=2

    if (__GET(i_priv) >= __GET(t)) {
        write_to_gbuf(&i_priv, &i, sizeof(i));
        NEXT(7);
    } else {
        write_to_gbuf(&i_priv, &i, sizeof(i));
        NEXT(2);
    }

#ifdef TSK_SIZ
    cp_sendRes ("ce_1 \0");
#endif
}

TEB(ce_2)//2
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    if (__GET(t) % __GET(i) == 0) {
        NEXT(1);
    } else {
        NEXT(3);
    }

#ifdef TSK_SIZ
    cp_sendRes ("ce_2 \0");
#endif
}

TEB(is_i_prime)//3
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    int c;
    c=sqrt16(__GET(i));
    __SET(j) = c;
    for(c=2; c <= __GET( j) ;c++)
    {
        if( __GET(i) % c==0)
        {
            __SET(flag)=0;
            NEXT(1);
            //return;
        }
    }
    __SET(flag) = 1;

#ifdef TSK_SIZ
    cp_sendRes ("is_i_prime \0");
#endif

    NEXT(4);
}

TEB(ce_3)//4
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    long int in_i = __GET(i);
    if( __GET(flag) == 1 && in_i != __GET(p) && in_i != __GET(q) )
    {
        __SET(e[__GET(k)]) = in_i ;
    } else {
        NEXT(1);
        //return;
    }

#ifdef TSK_SIZ
    cp_sendRes ("ce_3 \0");
#endif

    NEXT(5);
}

TEB(cd)//5
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    long int kk=1, __cry;
    while(1)
    {
        kk=kk +  __GET(t);
        if(kk % __GET( e[__GET(k)] ) ==0){
            __cry = (kk/ __GET( e[ __GET(k) ]) );
            __SET(flag) = __cry;
            break;
        }
    }

#ifdef TSK_SIZ
    cp_sendRes ("cd \0");
#endif

    NEXT(6);
}

TEB(ce_4)//6
//war k 
k_priv = __GET(k);
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

    int __cry = __GET(flag);
    if(__cry > 0)
    {
        __SET(d[ __GET(k_priv) ]) =__cry;
        __SET(k_priv)++;
    }

    if (__GET(k_priv) < 9) {
        write_to_gbuf(&k_priv, &k, sizeof(k));
        NEXT(1);
    } else {
        write_to_gbuf(&k_priv, &k, sizeof(k));
        NEXT(7);
    }

#ifdef TSK_SIZ
    cp_sendRes ("ce_4 \0");
#endif
}

TEB(encrypt_init)//7
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

  long int __cry;
   __cry = __GET(m[ __GET(en_cnt) ]) ;
   __SET(en_pt) = __cry;
   __SET(en_pt) -=96;
   __SET(en_k)  = 1;
   __SET(en_j)  = 0;
   __cry = __GET(e[0]) ;
   __SET(en_key) = __cry;

#ifdef TSK_SIZ
    cp_sendRes ("encrypt_init \0");
#endif

    NEXT(8);
}

TEB(encrypt_inner_loop)//8
//war en_k en_j
en_k_priv = __GET(en_k);
en_j_priv = __GET(en_j);
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

   long int __cry;
    if (__GET(en_j_priv) < __GET(en_key)) {
        __cry = __GET(en_k_priv) * __GET(en_pt);
        __SET(en_k_priv) = __cry;
        __cry = __GET(en_k_priv) % __GET(n);
        __SET(en_k_priv) = __cry;
        __SET(en_j_priv)++;
        //
        write_to_gbuf(&en_k_priv, &k, sizeof(k));
        write_to_gbuf(&en_j_priv, &j, sizeof(j));
        NEXT(8);
    } else {
        write_to_gbuf(&en_k_priv, &k, sizeof(k));
        write_to_gbuf(&en_j_priv, &j, sizeof(j));
        NEXT(9);
    }

#ifdef TSK_SIZ
    cp_sendRes ("encrypt_inner_loop \0");
#endif
}

TEB(encrypt_finish)//9
//war en_cnt 
en_cnt_priv = __GET(en_cnt);
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

   long int __cry;
   __cry = __GET(en_k);
   __SET(temp[ __GET(en_cnt_priv) ]) = __cry;
   __cry = __GET(en_k) + 96;
   __SET(en_ct) = __cry;
   __cry = __GET(en_ct);
   __SET(en[ __GET(en_cnt_priv) ]) = __cry;

    if (__GET(en_cnt_priv) < MSG_LEN) {
        __SET(en_cnt_priv)++;
        //
        write_to_gbuf(&en_cnt_priv, &en_cnt, sizeof(en_cnt));
        NEXT(7);
    } else {
        __SET(en[ __GET(en_cnt_priv) ]) = -1;
        //
        write_to_gbuf(&en_cnt_priv, &en_cnt, sizeof(en_cnt));
        NEXT(10);
    }

#ifdef TSK_SIZ
    cp_sendRes ("encrypt_finish \0");
#endif
}

TEB(encrypt_print)//10
{
#ifdef DEBUG
    uart_sendText("THE_ENCRYPTED_MESSAGE_IS\n\r", 26);
    for(en_cnt=0;en_cnt < MSG_LEN;en_cnt++){
        uart_sendChar(en[en_cnt]);
    }
    uart_sendText("\n\r", 2);
#endif

    NEXT(11);
}
TEB(decrypt_init)//11
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

   long int __cry;
   __SET(de_k)  = 1;
   __SET(de_j)  = 0;
   __cry =__GET(d[0]);
   __SET(de_key) = __cry;

#ifdef TSK_SIZ
    cp_sendRes ("decrypt_init \0");
#endif

    NEXT(12);
}

TEB(decrypt_inner_loop)//12
//war de_k de_j
de_k_priv = __GET(de_k);
de_j_priv = __GET(de_j);
{
#ifdef TSK_SIZ
    cp_reset ();
#endif
   long int __cry;
   __cry =  __GET(temp[ __GET(de_cnt) ]);
   __SET(de_ct) = __cry;

    if( __GET(de_j_priv) < __GET(de_key) )
    {
        __cry = __GET(de_k_priv) * __GET(de_ct);
        __SET(de_k_priv) = __cry;
        __cry = __GET(de_k_priv) % __GET(n);
        __SET(de_k_priv) = __cry;
        __SET(de_j_priv)++;
        //
        write_to_gbuf(&de_k_priv, &de_k, sizeof(de_k));
        write_to_gbuf(&de_j_priv, &de_j, sizeof(de_j));
        NEXT(12);
    } else {
        write_to_gbuf(&de_k_priv, &de_k, sizeof(de_k));
        write_to_gbuf(&de_j_priv, &de_j, sizeof(de_j));
        NEXT(13);
    }

#ifdef TSK_SIZ
    cp_sendRes ("decrypt_inner_loop \0");
#endif

}

TEB(decrypt_finish)//13
//war de_cnt
de_cnt_priv = __GET(de_cnt);

//long int de_cnt;
{
#ifdef TSK_SIZ
    cp_reset ();
#endif

   long int __cry;
   __cry = __GET(de_k) + 96;
   __SET(de_pt) = __cry;
   __SET(m[ __GET(de_cnt_priv)]) = __cry;

    if (__GET(en[ __GET(de_cnt_priv) ]) != -1) {
        __SET(de_cnt_priv)++;
        //
        write_to_gbuf(&de_cnt_priv, &de_cnt, sizeof(de_cnt));
        NEXT(11);
    } else {
        write_to_gbuf(&de_cnt_priv, &de_cnt, sizeof(de_cnt));
        NEXT(14);
    }

#ifdef TSK_SIZ
    cp_sendRes ("decrypt_print \0");
#endif
}

TEB(decrypt_print)//14
{
    if (full_run_started) {
        full_run_started = 0;
    }

    NEXT(0);
}

void _benchmark_rsa_init()
{
    TASK(TASK_PRI);

    TEB_INIT(TASK_PRI, initTask,            3, may_war_set_rsa[0][0],   may_war_set_rsa[0][1],  teb_breaking_rsa[0]);       //0
    TEB_INIT(TASK_PRI, ce_1,                1, may_war_set_rsa[1][0],   may_war_set_rsa[1][1],  teb_breaking_rsa[1]); //1
    TEB_INIT(TASK_PRI, ce_2,                1, may_war_set_rsa[2][0],   may_war_set_rsa[2][1],  teb_breaking_rsa[2]); //2
    TEB_INIT(TASK_PRI, is_i_prime,          4, may_war_set_rsa[3][0],   may_war_set_rsa[3][1],  teb_breaking_rsa[3]);    //3
    TEB_INIT(TASK_PRI, ce_3,                1, may_war_set_rsa[4][0],   may_war_set_rsa[4][1],  teb_breaking_rsa[4]);  //4
    TEB_INIT(TASK_PRI, cd,                  5, may_war_set_rsa[5][0],   may_war_set_rsa[5][1],  teb_breaking_rsa[5]); //5
    TEB_INIT(TASK_PRI, ce_4,                1, may_war_set_rsa[6][0],   may_war_set_rsa[6][1],  teb_breaking_rsa[6]);  //6
    TEB_INIT(TASK_PRI, encrypt_init,        1, may_war_set_rsa[7][0],   may_war_set_rsa[7][1],  teb_breaking_rsa[7]);      //7
    TEB_INIT(TASK_PRI, encrypt_inner_loop,  1, may_war_set_rsa[8][0],   may_war_set_rsa[8][1],  teb_breaking_rsa[8]);    //8
    TEB_INIT(TASK_PRI, encrypt_finish,      1, may_war_set_rsa[9][0],   may_war_set_rsa[9][1],  teb_breaking_rsa[9]);      //9
    TEB_INIT(TASK_PRI, encrypt_print,       1, may_war_set_rsa[10][0],  may_war_set_rsa[10][1], teb_breaking_rsa[10]);     //10
    TEB_INIT(TASK_PRI, decrypt_init,        1, may_war_set_rsa[11][0],  may_war_set_rsa[11][1], teb_breaking_rsa[11]);      //7
    TEB_INIT(TASK_PRI, decrypt_inner_loop,  1, may_war_set_rsa[12][0],  may_war_set_rsa[12][1], teb_breaking_rsa[12]);    //8
    TEB_INIT(TASK_PRI, decrypt_finish,      1, may_war_set_rsa[13][0],  may_war_set_rsa[13][1], teb_breaking_rsa[13]);      //9
    TEB_INIT(TASK_PRI, decrypt_print,       1, may_war_set_rsa[14][0],  may_war_set_rsa[14][1], teb_breaking_rsa[14]);     //10

    __SIGNAL(1);
}

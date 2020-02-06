#include "util/log.h"
#include "util/debug.h"
#include "util/mem.h"
#include "eth_api.h"   //wrapper for easier use
#include "eth_basic.h" // the full ethereum verifier containing the EVM
//#include "eth_call_response_binary.h"
#include "block_number.h"
#include "receipt.h"
#define TEST_Z
#ifdef TEST_Z
#include <ztest.h>
#endif
/**
 * In3 Setup and usage
 * **/
/* Perform in3 requests for http transport */
in3_ret_t local_transport_func(char **urls, int urls_len, char *payload, in3_response_t *result) {
  for (int i=0; i<urls_len; i++) {
    if (strstr(payload, "eth_getTransactionReceipt") != NULL) {
      printk("Returning eth_getTransactionReceipt ...\n");
      sb_add_range(&(result[i].result), mock_tx_receipt, 0, mock_tx_receipt_len);
    }
    else if (strstr(payload, "eth_blockNumber") != NULL) {
      printk("Returning eth_blockNumber ...\n");
      sb_add_range(&(result[i].result), block_number_res, 0, block_number_res_len);
		}
    else {
			printk("Not supported for this mock\n");
		}     
    //sb_add_range(&(result[i].result), mock_eth_call_response, 0, mock_eth_call_response_len);
    
  }
  return IN3_OK;
}

in3_ret_t transport_mock(in3_request_t* req){
  return local_transport_func((char**) req->urls, req->urls_len, req->payload, req->results);
}

in3_t* init_in3(in3_transport_send custom_transport, chain_id_t chain) {
  in3_t* in3 = NULL;
  //int    err;
  in3_register_eth_basic();
  in3_log_set_quiet(0);
  in3_log_set_level(LOG_TRACE);
  in3 = in3_for_chain(0);
  if (custom_transport)
    in3->transport = custom_transport; // use curl to handle the requests
  in3->request_count    = 1;           // number of requests to sendp
  in3->include_code     = 1;
  in3->max_attempts     = 1;
  in3->request_count    = 1; // number of requests to sendp
  in3->chain_id         = chain;  
  in3->auto_update_list = false;
  in3->use_binary = true;
  for (int i = 0; i < in3->chains_length; i++) in3->chains[i].nodelist_upd8_params = NULL;
  return in3;
}


static void test_block_number() {
  in3_t*   in3    = init_in3(transport_mock, 0x5);
  uint64_t blknum = eth_blockNumber(in3);
  printk("BLKNUM : %lld \n", blknum);
  zassert_true(blknum > 0 , "BLK TEST FAILED");
  in3_free(in3);
}




#ifdef TEST_Z
void test_main(void)
{
	ztest_test_suite(framework_tests,
		ztest_unit_test(test_block_number)
	);

	ztest_run_test_suite(framework_tests);
}
#else
void main() {
  // the hash of transaction whose receipt we want to get

  in3_t*    in3 = init_in3(transport_mock, 0x5);
  bytes32_t tx_hash;
  hex_to_bytes("0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d", -1, tx_hash, 32);

  // get the tx receipt by hash
  eth_tx_receipt_t* txr = eth_getTransactionReceipt(in3, tx_hash);
  printk("status %d", txr->status);
  printk("gas %d", txr->gas_used > 0);
  eth_tx_receipt_free(txr);
  in3_free(in3);
}
#endif


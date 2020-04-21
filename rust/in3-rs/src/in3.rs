use std::ffi;
use std::ffi::{CString, CStr};
use async_trait::async_trait;

use crate::error::{Error, In3Result};
use crate::traits::{Client as ClientTrait, Storage, Transport};
use crate::transport::HttpTransport;

pub mod chain {
    pub type ChainId = u32;

    pub const MULTICHAIN: u32 = 0x0;
    pub const MAINNET: u32 = 0x01;
    pub const KOVAN: u32 = 0x2a;
    pub const TOBALABA: u32 = 0x44d;
    pub const GOERLI: u32 = 0x5;
    pub const EVAN: u32 = 0x4b1;
    pub const IPFS: u32 = 0x7d0;
    pub const BTC: u32 = 0x99;
    pub const LOCAL: u32 = 0xffff;
}

pub struct Ctx {
    ptr: *mut in3_sys::in3_ctx_t,
    #[allow(dead_code)]
    config: ffi::CString,
}

impl Ctx {
    pub fn new(in3: &mut Client, config_str: &str) -> Ctx {
        let config = ffi::CString::new(config_str).expect("CString::new failed");
        let ptr: *mut in3_sys::in3_ctx_t;
        unsafe { ptr = in3_sys::ctx_new(in3.ptr, config.as_ptr()); }
        Ctx { ptr, config }
    }

    pub fn sign(&mut self, type_: u8, data:*const u8, len: u32) -> CString {
        unsafe {
            let pk = (*(*(*self.ptr).client).signer).wallet as *mut u8;
            let dst  = libc::malloc(65) as *mut u8;
            let pby = *dst.offset(64) as *mut u8;
            // let pby = dst.offset(64) as *mut u8;
            let curve = in3_sys::secp256k1;
            // let type_sys = type_ as in3_sys::d_signature_type_t;
            let mut error: libc::c_int = 0;
            let enm_type: in3_sys::d_signature_type_t = match type_ {
                0 => in3_sys::d_signature_type_t::SIGN_EC_RAW,
                1 => in3_sys::d_signature_type_t::SIGN_EC_HASH,
                _ => panic!("Unknown value: {}", type_),
            };
            match enm_type {
               in3_sys::d_signature_type_t::SIGN_EC_RAW => {
                    error = in3_sys::ecdsa_sign_digest(&curve, pk, data, dst, pby, None);
               }
               in3_sys::d_signature_type_t::SIGN_EC_HASH => {
                    error = in3_sys::ecdsa_sign(&curve, in3_sys::HasherType::HASHER_SHA3K, pk, data, len, dst, pby, None);
               }
            }
            let mut value = std::slice::from_raw_parts_mut(dst, 65 as usize);
            // let str_sign = CStr::from_ptr(dst);
            let str_sign = CString::from_vec_unchecked(value.to_vec());
            println!("\n{:?}", str_sign);
            // for byte in value {
            //     print!("{:x}", byte);
            // }
            str_sign
            
        }
    }

    pub async unsafe fn execute(&mut self) -> In3Result<String> {
        loop {
            let mut ctx_ret = in3_sys::in3_ctx_execute(self.ptr);
            let mut last_waiting: *mut in3_sys::in3_ctx_t;
            let mut p: *mut in3_sys::in3_ctx_t;
            last_waiting = (*self.ptr).required;
            p = self.ptr;
            match ctx_ret {
                in3_sys::in3_ret_t::IN3_EIGNORE => {
                    while p != std::ptr::null_mut() {
                        let p_req = (*p).required;
                        if p_req != std::ptr::null_mut()
                            && (*p_req).verification_state == in3_sys::in3_ret_t::IN3_EIGNORE
                        {
                            last_waiting = p;
                        }
                        p = (*last_waiting).required;
                    }
                    if last_waiting == std::ptr::null_mut() {
                        return Err("Cound not find the last waiting context".into());
                    } else {
                        in3_sys::ctx_handle_failable(last_waiting);
                        return Err(Error::TryAgain);
                    }
                }
                in3_sys::in3_ret_t::IN3_WAITING => {
                    while p != std::ptr::null_mut() {
                        let state = in3_sys::in3_ctx_state(p);
                        let res = (*p).raw_response;
                        if res == std::ptr::null_mut()
                            && state == in3_sys::state::CTX_WAITING_FOR_RESPONSE
                        {
                            last_waiting = p;
                        }
                        p = (*last_waiting).required;
                    }
                    if last_waiting == std::ptr::null_mut() {
                        return Err("Cound not find the last waiting context".into());
                    }
                }
                in3_sys::in3_ret_t::IN3_OK => {
                    let result = (*(*self.ptr).response_context).c;
                    let data = ffi::CStr::from_ptr(result).to_str().unwrap();
                    return Ok(data.into());
                }
                err => {
                    return Err(err.into());
                }
            }

            if last_waiting != std::ptr::null_mut() {
                let req_type = (*last_waiting).type_;
                match req_type {
                    in3_sys::ctx_type::CT_SIGN => {
                        // unimplemented!();
                        let req = in3_sys::in3_create_request(last_waiting);
                        let data = (*req).payload as *mut u8;
                        let res_str = self.sign(0, data, 32);
                        in3_sys::sb_add_chars(
                            &mut (*(*req).results.add(0)).result,
                            res_str.into_raw(),
                        );
                        let result = (*(*req).results.offset(0)).result;
                        let len = result.len;
                        let data = ffi::CStr::from_ptr(result.data).to_str().unwrap();
                        println!("DATA -- > {}", data); 
                        // println!("TODO CT_SIGN");
                        return Ok(data.to_string());
                    }
                    in3_sys::ctx_type::CT_RPC => {
                        let req = in3_sys::in3_create_request(last_waiting);
                        let payload = ffi::CStr::from_ptr((*req).payload).to_str().unwrap();
                        let urls_len = (*req).urls_len;
                        let mut urls = Vec::new();
                        for i in 0..urls_len as usize {
                            let url = ffi::CStr::from_ptr(*(*req).urls.add(i))
                                .to_str()
                                .unwrap();
                            urls.push(url);
                        }

                        let responses: Vec<Result<String, String>> = {
                            let mut transport = {
                                let c = (*(*last_waiting).client).internal as *mut Client;
                                &mut (*c).transport
                            };
                            transport.fetch(payload, &urls).await
                        };
                        for (i, resp) in responses.iter().enumerate() {
                            match resp {
                                Err(err) => {
                                    let err_str = ffi::CString::new(err.to_string()).unwrap();
                                    in3_sys::sb_add_chars(
                                        &mut (*(*req).results.add(i)).error,
                                        err_str.as_ptr(),
                                    );
                                }
                                Ok(res) => {
                                    let res_str = ffi::CString::new(res.to_string()).unwrap();
                                    in3_sys::sb_add_chars(
                                        &mut (*(*req).results.add(i)).result,
                                        res_str.as_ptr(),
                                    );
                                }
                            }
                        }
                        let result = (*(*req).results.offset(0)).result;
                        let len = result.len;
                        let data = ffi::CStr::from_ptr(result.data).to_str().unwrap();
                        if len != 0 {
                            return Err(Error::TryAgain);
                        } else {
                            let error = (*(*req).results.offset(0)).error;
                            let err = ffi::CStr::from_ptr(error.data).to_str().unwrap();
                            return Err(err.into());
                        }
                    }
                }
            }
        };
    }

    #[cfg(feature = "blocking")]
    pub fn send(&mut self) -> In3Result<()> {
        unsafe {
            let ret = in3_sys::in3_send_ctx(self.ptr);
            match ret {
                in3_sys::in3_ret_t::IN3_OK => Ok(()),
                _ => Err(ret.into()),
            }
        }
    }
}

impl Drop for Ctx {
    fn drop(&mut self) {
        unsafe {
            in3_sys::ctx_free(self.ptr);
        }
    }
}

pub struct Request {
    ptr: *mut in3_sys::in3_request_t,
    ctx_ptr: *const in3_sys::in3_ctx_t,
}

impl Request {
    pub fn new(ctx: &mut Ctx) -> Request {
        unsafe {
            Request {
                ptr: in3_sys::in3_create_request(ctx.ptr),
                ctx_ptr: ctx.ptr,
            }
        }
    }
}

impl Drop for Request {
    fn drop(&mut self) {
        unsafe {
            in3_sys::request_free(self.ptr, self.ctx_ptr, false);
        }
    }
}

pub struct Client {
    ptr: *mut in3_sys::in3_t,
    transport: Box<dyn Transport>,
    storage: Option<Box<dyn Storage>>,
}

#[async_trait(? Send)]
impl ClientTrait for Client {
    fn configure(&mut self, config: &str) -> Result<(), String> {
        unsafe {
            let config_c = ffi::CString::new(config).expect("CString::new failed");
            let err = in3_sys::in3_configure(self.ptr, config_c.as_ptr());
            if err.as_ref().is_some() {
                return Err(ffi::CStr::from_ptr(err).to_str().unwrap().to_string());
            }
        }
        Ok(())
    }

    fn set_transport(&mut self, transport: Box<dyn Transport>) {
        self.transport = transport;
    }

    fn set_storage(&mut self, storage: Box<dyn Storage>) {
        let no_storage = self.storage.is_none();
        self.storage = Some(storage);
        if no_storage {
            unsafe {
                (*self.ptr).cache = in3_sys::in3_set_storage_handler(self.ptr, Some(Client::in3_rust_storage_get),
                                                                     Some(Client::in3_rust_storage_set),
                                                                     Some(Client::in3_rust_storage_clear),
                                                                     self.ptr as *mut libc::c_void);
            }
        }
    }

    async fn rpc(&mut self, call: &str) -> In3Result<String> {
        let mut ctx = Ctx::new(self, call);
        loop {
            let res = unsafe { ctx.execute().await };
            if res != Err(Error::TryAgain) {
                return res;
            }
        }
    }

    #[cfg(feature = "blocking")]
    fn rpc_blocking(&mut self, call: &str) -> In3Result<String> {
        let mut null: *mut i8 = std::ptr::null_mut();
        let res: *mut *mut i8 = &mut null;
        let err: *mut *mut i8 = &mut null;
        let req_str = ffi::CString::new(call).unwrap();
        unsafe {
            let ret = in3_sys::in3_client_rpc_raw(self.ptr, req_str.as_ptr(), res, err);
            match ret {
                in3_sys::in3_ret_t::IN3_OK => Ok(ffi::CStr::from_ptr(*res).to_str().unwrap().to_string()),
                _ => Err(Error::CustomError(ffi::CStr::from_ptr(*err).to_str().unwrap().to_string()))
            }
        }
    }
    fn hex_to_bytes(&mut self, data: &str) -> *mut u8{
        unsafe {
            let c_str_data = CString::new(data).unwrap(); // from a &str, creates a new allocation
            let c_data: *const c_char = c_str_data.as_ptr();
            let mut out:*mut u8 = libc::malloc(strlen(c_data) as usize) as *mut u8;
            let len:i32 = -1;
            in3_sys::hex_to_bytes(c_data, len, out, 32);
            let data_ = std::slice::from_raw_parts_mut(out, 32 as usize);
            for byte in data_ {
                print!("{:x}", byte);
            }
            println!("\n");
            out
        }
    }
    fn new_bytes(&mut self, data: &str) -> *mut u8 {
        unsafe {
        let c_str_data = CString::new(data).unwrap(); // from a &str, creates a new allocation
        let data_ptr= c_str_data.as_ptr();
        let len= strlen(data_ptr) as i32;
        let data = in3_sys::hex_to_new_bytes(data_ptr, len);
        let data_ = (*data).data;
        let out = std::slice::from_raw_parts_mut(data_, 32 as usize);
        for byte in out {
            print!("{:x}", byte);
        }
        println!("\n");
        data_
        }
    }
    fn set_pk_signer(&mut self, data: &str) {
        unsafe {
            let pk_ = self.hex_to_bytes(data);
            in3_sys::eth_set_pk_signer(self.ptr, pk_);
        }
    }
}

impl Client {
    pub fn new(chain_id: chain::ChainId) -> Box<Client> {
        unsafe {
            let mut c = Box::new(Client {
                ptr: in3_sys::in3_for_chain_auto_init(chain_id),
                transport: Box::new(HttpTransport {}),
                storage: None,
            });
            let c_ptr: *mut ffi::c_void = &mut *c as *mut _ as *mut ffi::c_void;
            (*c.ptr).internal = c_ptr;
            #[cfg(feature = "blocking")] {
                (*c.ptr).transport = Some(Client::in3_rust_transport);
            }
            c
        }
    }
    unsafe extern "C" fn in3_rust_storage_get(
        cptr: *mut libc::c_void,
        key: *const libc::c_char,
    ) -> *mut in3_sys::bytes_t {
        let key = ffi::CStr::from_ptr(key).to_str().unwrap();
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &(*c).storage {
            if let Some(val) = storage.get(key) {
                return in3_sys::b_new(val.as_ptr(), val.len() as u32);
            }
        }
        std::ptr::null_mut()
    }

    unsafe extern "C" fn in3_rust_storage_set(
        cptr: *mut libc::c_void,
        key: *const libc::c_char,
        value: *mut in3_sys::bytes_t,
    ) {
        let key = ffi::CStr::from_ptr(key).to_str().unwrap();
        let value = std::slice::from_raw_parts_mut((*value).data, (*value).len as usize);
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &mut (*c).storage {
            storage.set(key, value);
        }
    }

    unsafe extern "C" fn in3_rust_storage_clear(cptr: *mut libc::c_void) {
        let client = cptr as *mut in3_sys::in3_t;
        let c = (*client).internal as *mut Client;
        if let Some(storage) = &mut (*c).storage {
            storage.clear();
        }
    }

    #[cfg(feature = "blocking")]
    extern "C" fn in3_rust_transport(
        request: *mut in3_sys::in3_request_t,
    ) -> in3_sys::in3_ret_t::Type {
        // internally calls the rust transport impl, i.e. Client.transport
        let mut urls = Vec::new();

        unsafe {
            let payload = ffi::CStr::from_ptr((*request).payload).to_str().unwrap();
            let urls_len = (*request).urls_len;
            for i in 0..urls_len as usize {
                let url = ffi::CStr::from_ptr(*(*request).urls.add(i))
                    .to_str()
                    .unwrap();
                urls.push(url);
            }

            let c = (*(*request).in3).internal as *mut Client;
            let responses: Vec<Result<String, String>> = (*c).transport.fetch_blocking(payload, &urls);

            let mut any_err = false;
            for (i, resp) in responses.iter().enumerate() {
                match resp {
                    Err(err) => {
                        any_err = true;
                        let err_str = ffi::CString::new(err.to_string()).unwrap();
                        in3_sys::sb_add_chars(
                            &mut (*(*request).results.add(i)).error,
                            err_str.as_ptr(),
                        );
                    }
                    Ok(res) => {
                        let res_str = ffi::CString::new(res.to_string()).unwrap();
                        in3_sys::sb_add_chars(
                            &mut (*(*request).results.add(i)).result,
                            res_str.as_ptr(),
                        );
                    }
                }
            }

            if urls_len as usize != responses.len() || any_err {
                return in3_sys::in3_ret_t::IN3_ETRANS;
            }
        }

        in3_sys::in3_ret_t::IN3_OK
    }
}

impl Drop for Client {
    fn drop(&mut self) {
        unsafe {
            in3_sys::in3_free(self.ptr);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_in3_config() {
        let mut in3 = Client::new(chain::MAINNET);
        let c = in3.configure(r#"{"autoUpdateList":false}"#);
        assert_eq!(c.is_err(), false);
    }
}

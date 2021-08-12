use crate::bindings::root as bindings;
use crate::btif::{
    ptr_to_vec, BluetoothInterface, BtStatus, FfiAddress, RawAddress, SupportedProfiles, Uuid,
};
use crate::profiles::gatt::bindings::{
    btgatt_callbacks_t, btgatt_client_callbacks_t, btgatt_client_interface_t, btgatt_interface_t,
    btgatt_scanner_callbacks_t, btgatt_server_callbacks_t, btgatt_server_interface_t,
    BleAdvertiserInterface, BleScannerInterface,
};
use crate::topstack::get_dispatchers;
use crate::{cast_to_ffi_address, ccall, deref_ffi_address};

use std::sync::{Arc, Mutex};

use topshim_macros::cb_variant;

pub type BtGattNotifyParams = bindings::btgatt_notify_params_t;
pub type BtGattReadParams = bindings::btgatt_read_params_t;
pub type BtGattDbElement = bindings::btgatt_db_element_t;
pub type BtGattResponse = bindings::btgatt_response_t;
pub type BtGattTestParams = bindings::btgatt_test_params_t;

// TODO: Implement printing internal values.
impl std::fmt::Debug for BtGattReadParams {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("BtGattReadParams"))
    }
}

// TODO: Implement printing internal values.
impl std::fmt::Debug for BtGattNotifyParams {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("BtGattNotifyParams"))
    }
}

#[derive(Debug)]
pub enum GattClientCallbacks {
    RegisterClient(i32, i32, Uuid),
    Connect(i32, i32, i32, RawAddress),
    Disconnect(i32, i32, i32, RawAddress),
    SearchComplete(i32, i32),
    RegisterForNotification(i32, i32, i32, u16),
    Notify(i32, BtGattNotifyParams),
    ReadCharacteristic(i32, i32, BtGattReadParams),
    WriteCharacteristic(i32, i32, u16),
    ReadDescriptor(i32, i32, BtGattReadParams),
    WriteDescriptor(i32, i32, u16),
    ExecuteWrite(i32, i32),
    ReadRemoteRssi(i32, RawAddress, i32, i32),
    ConfigureMtu(i32, i32, i32),
    Congestion(i32, bool),
    GetGattDb(i32, Vec<BtGattDbElement>, i32),
    ServicesRemoved(i32, u16, u16),
    ServicesAdded(i32, Vec<BtGattDbElement>, i32),
    PhyUpdated(i32, u8, u8, u8),
    ConnUpdated(i32, u16, u16, u16, u8),
    ServiceChanged(i32),
}

#[derive(Debug)]
pub enum GattServerCallbacks {
    RegisterServer(i32, i32, Uuid),
    Connection(i32, i32, i32, RawAddress),
    ServiceAdded(i32, i32, Vec<BtGattDbElement>, usize),
    ServiceStopped(i32, i32, i32),
    ServiceDeleted(i32, i32, i32),
    RequestReadCharacteristic(i32, i32, RawAddress, i32, i32, bool),
    RequestReadDescriptor(i32, i32, RawAddress, i32, i32, bool),
    RequestWriteCharacteristic(i32, i32, RawAddress, i32, i32, bool, bool, Vec<u8>, usize),
    RequestWriteDescriptor(i32, i32, RawAddress, i32, i32, bool, bool, Vec<u8>, usize),
    RequestExecWrite(i32, i32, RawAddress, i32),
    ResponseConfirmation(i32, i32),
    IndicationSent(i32, i32),
    Congestion(i32, bool),
    MtuChanged(i32, i32),
    PhyUpdated(i32, u8, u8, u8),
    ConnUpdated(i32, u16, u16, u16, u8),
}

pub struct GattClientCallbacksDispatcher {
    pub dispatch: Box<dyn Fn(GattClientCallbacks) + Send>,
}

pub struct GattServerCallbacksDispatcher {
    pub dispatch: Box<dyn Fn(GattServerCallbacks) + Send>,
}

type GattClientCb = Arc<Mutex<GattClientCallbacksDispatcher>>;
type GattServerCb = Arc<Mutex<GattServerCallbacksDispatcher>>;

cb_variant!(
    GattClientCb,
    gc_register_client_cb -> GattClientCallbacks::RegisterClient,
    i32, i32, *const Uuid, {
        let _2 = unsafe { *_2.clone() };
    }
);

cb_variant!(
    GattClientCb,
    gc_open_cb -> GattClientCallbacks::Connect,
    i32, i32, i32, *const FfiAddress, {
        let _3 = unsafe { deref_ffi_address!(_3) };
    }
);

cb_variant!(
    GattClientCb,
    gc_close_cb -> GattClientCallbacks::Disconnect,
    i32, i32, i32, *const FfiAddress, {
        let _3 = unsafe { deref_ffi_address!(_3) };
    }
);

cb_variant!(
    GattClientCb,
    gc_search_complete_cb -> GattClientCallbacks::SearchComplete,
    i32, i32, {}
);

cb_variant!(
    GattClientCb,
    gc_register_for_notification_cb -> GattClientCallbacks::RegisterForNotification,
    i32, i32, i32, u16, {}
);

cb_variant!(
    GattClientCb,
    gc_notify_cb -> GattClientCallbacks::Notify,
    i32, *const BtGattNotifyParams, {
        let _1 = unsafe { *_1.clone() };
    }
);

cb_variant!(
    GattClientCb,
    gc_read_characteristic_cb -> GattClientCallbacks::ReadCharacteristic,
    i32, i32, *mut BtGattReadParams, {
        let _2 = unsafe { *_2.clone() };
    }
);

cb_variant!(
    GattClientCb,
    gc_write_characteristic_cb -> GattClientCallbacks::WriteCharacteristic,
    i32, i32, u16, {}
);

cb_variant!(
    GattClientCb,
    gc_read_descriptor_cb -> GattClientCallbacks::ReadDescriptor,
    i32, i32, *const BtGattReadParams, {
        let _2 = unsafe { *_2.clone() };
    }
);

cb_variant!(
    GattClientCb,
    gc_write_descriptor_cb -> GattClientCallbacks::WriteDescriptor,
    i32, i32, u16, {}
);

cb_variant!(
    GattClientCb,
    gc_execute_write_cb -> GattClientCallbacks::ExecuteWrite,
    i32, i32, {}
);

cb_variant!(
    GattClientCb,
    gc_read_remote_rssi_cb -> GattClientCallbacks::ReadRemoteRssi,
    i32, *const FfiAddress, i32, i32, {
        let _1 = unsafe { deref_ffi_address!(_1) };
    }
);

cb_variant!(
    GattClientCb,
    gc_configure_mtu_cb -> GattClientCallbacks::ConfigureMtu,
    i32, i32, i32, {}
);

cb_variant!(
    GattClientCb,
    gc_congestion_cb -> GattClientCallbacks::Congestion,
    i32, bool, {}
);

cb_variant!(
    GattClientCb,
    gc_get_gatt_db_cb -> GattClientCallbacks::GetGattDb,
    i32, *const BtGattDbElement, i32, {
        let _1 = ptr_to_vec(_1, _2 as usize);
    }
);

cb_variant!(
    GattClientCb,
    gc_services_removed_cb -> GattClientCallbacks::ServicesRemoved,
    i32, u16, u16, {}
);

cb_variant!(
    GattClientCb,
    gc_services_added_cb -> GattClientCallbacks::ServicesAdded,
    i32, *const BtGattDbElement, i32, {
        let _1 = ptr_to_vec(_1, _2 as usize);
    }
);

cb_variant!(
    GattClientCb,
    gc_phy_updated_cb -> GattClientCallbacks::PhyUpdated,
    i32, u8, u8, u8, {}
);

cb_variant!(
    GattClientCb,
    gc_conn_updated_cb -> GattClientCallbacks::ConnUpdated,
    i32, u16, u16, u16, u8, {}
);

cb_variant!(
    GattClientCb,
    gc_service_changed_cb -> GattClientCallbacks::ServiceChanged,
    i32, {}
);

cb_variant!(
    GattServerCb,
    gs_register_server_cb -> GattServerCallbacks::RegisterServer,
    i32, i32, *const Uuid, {
        let _2 = unsafe { *_2.clone() };
    }
);

cb_variant!(
    GattServerCb,
    gs_connection_cb -> GattServerCallbacks::Connection,
    i32, i32, i32, *const FfiAddress, {
        let _3 = unsafe { deref_ffi_address!(_3) };
    }
);

cb_variant!(
    GattServerCb,
    gs_service_added_cb -> GattServerCallbacks::ServiceAdded,
    i32, i32, *const BtGattDbElement, usize, {
        let _2 = ptr_to_vec(_2, _3);
    }
);

cb_variant!(
    GattServerCb,
    gs_service_stopped_cb -> GattServerCallbacks::ServiceStopped,
    i32, i32, i32, {}
);

cb_variant!(
    GattServerCb,
    gs_service_deleted_cb -> GattServerCallbacks::ServiceDeleted,
    i32, i32, i32, {}
);

cb_variant!(
    GattServerCb,
    gs_request_read_characteristic_cb -> GattServerCallbacks::RequestReadCharacteristic,
    i32, i32, *const FfiAddress, i32, i32, bool, {
        let _2 = unsafe { deref_ffi_address!(_2) };
    }
);

cb_variant!(
    GattServerCb,
    gs_request_read_descriptor_cb -> GattServerCallbacks::RequestReadDescriptor,
    i32, i32, *const FfiAddress, i32, i32, bool, {
        let _2 = unsafe { deref_ffi_address!(_2) };
    }
);

cb_variant!(
    GattServerCb,
    gs_request_write_characteristic_cb -> GattServerCallbacks::RequestWriteCharacteristic,
    i32, i32, *const FfiAddress, i32, i32, bool, bool, *const u8, usize, {
        let _2 = unsafe { deref_ffi_address!(_2) };
        let _7 = ptr_to_vec(_7, _8);
    }
);

cb_variant!(
    GattServerCb,
    gs_request_write_descriptor_cb -> GattServerCallbacks::RequestWriteDescriptor,
    i32, i32, *const FfiAddress, i32, i32, bool, bool, *const u8, usize, {
        let _2 = unsafe { deref_ffi_address!(_2) };
        let _7 = ptr_to_vec(_7, _8);
    }
);

cb_variant!(
    GattServerCb,
    gs_request_exec_write_cb -> GattServerCallbacks::RequestExecWrite,
    i32, i32, *const FfiAddress, i32, {
        let _2 = unsafe { deref_ffi_address!(_2) };
    }
);

cb_variant!(
    GattServerCb,
    gs_response_confirmation_cb -> GattServerCallbacks::ResponseConfirmation,
    i32, i32, {}
);

cb_variant!(
    GattServerCb,
    gs_indication_sent_cb -> GattServerCallbacks::IndicationSent,
    i32, i32, {}
);

cb_variant!(
    GattServerCb,
    gs_congestion_cb -> GattServerCallbacks::Congestion,
    i32, bool, {}
);

cb_variant!(
    GattServerCb,
    gs_mtu_changed_cb -> GattServerCallbacks::MtuChanged,
    i32, i32, {}
);

cb_variant!(
    GattServerCb,
    gs_phy_updated_cb -> GattServerCallbacks::PhyUpdated,
    i32, u8, u8, u8, {}
);

cb_variant!(
    GattServerCb,
    gs_conn_updated_cb -> GattServerCallbacks::ConnUpdated,
    i32, u16, u16, u16, u8, {}
);

struct RawGattWrapper {
    raw: *const btgatt_interface_t,
}

struct RawGattClientWrapper {
    raw: *const btgatt_client_interface_t,
}

struct RawGattServerWrapper {
    raw: *const btgatt_server_interface_t,
}

struct RawBleScannerWrapper {
    _raw: *const BleScannerInterface,
}

struct RawBleAdvertiserWrapper {
    _raw: *const BleAdvertiserInterface,
}

// Pointers unsafe due to ownership but this is a static pointer so Send is ok
unsafe impl Send for RawGattWrapper {}
unsafe impl Send for RawGattClientWrapper {}
unsafe impl Send for RawGattServerWrapper {}
unsafe impl Send for RawBleScannerWrapper {}
unsafe impl Send for RawBleAdvertiserWrapper {}
unsafe impl Send for btgatt_callbacks_t {}

pub struct GattClient {
    internal: RawGattClientWrapper,
}

impl GattClient {
    pub fn register_client(&self, uuid: &Uuid, eatt_support: bool) -> BtStatus {
        BtStatus::from(ccall!(self, register_client, uuid, eatt_support))
    }

    pub fn unregister_client(&self, client_if: i32) -> BtStatus {
        BtStatus::from(ccall!(self, unregister_client, client_if))
    }

    pub fn connect(
        &self,
        client_if: i32,
        addr: &RawAddress,
        is_direct: bool,
        transport: i32,
        opportunistic: bool,
        initiating_phys: i32,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(
            self,
            connect,
            client_if,
            ffi_addr,
            is_direct,
            transport,
            opportunistic,
            initiating_phys
        ))
    }

    pub fn disconnect(&self, client_if: i32, addr: &RawAddress, conn_id: i32) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, disconnect, client_if, ffi_addr, conn_id))
    }

    pub fn refresh(&self, client_if: i32, addr: &RawAddress) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, refresh, client_if, ffi_addr))
    }

    pub fn search_service(&self, conn_id: i32, filter_uuid: &Uuid) -> BtStatus {
        BtStatus::from(ccall!(self, search_service, conn_id, filter_uuid))
    }

    pub fn btif_gattc_discover_service_by_uuid(&self, conn_id: i32, uuid: &Uuid) {
        ccall!(self, btif_gattc_discover_service_by_uuid, conn_id, uuid)
    }

    pub fn read_characteristic(&self, conn_id: i32, handle: u16, auth_req: i32) -> BtStatus {
        BtStatus::from(ccall!(self, read_characteristic, conn_id, handle, auth_req))
    }

    pub fn read_using_characteristic_uuid(
        &self,
        conn_id: i32,
        uuid: &Uuid,
        s_handle: u16,
        e_handle: u16,
        auth_req: i32,
    ) -> BtStatus {
        BtStatus::from(ccall!(
            self,
            read_using_characteristic_uuid,
            conn_id,
            uuid,
            s_handle,
            e_handle,
            auth_req
        ))
    }

    pub fn write_characteristic(
        &self,
        conn_id: i32,
        handle: u16,
        write_type: i32,
        auth_req: i32,
        value: &[u8],
    ) -> BtStatus {
        BtStatus::from(ccall!(
            self,
            write_characteristic,
            conn_id,
            handle,
            write_type,
            auth_req,
            value.as_ptr(),
            value.len()
        ))
    }

    pub fn read_descriptor(&self, conn_id: i32, handle: u16, auth_req: i32) -> BtStatus {
        BtStatus::from(ccall!(self, read_descriptor, conn_id, handle, auth_req))
    }

    pub fn write_descriptor(
        &self,
        conn_id: i32,
        handle: u16,
        auth_req: i32,
        value: &[u8],
    ) -> BtStatus {
        BtStatus::from(ccall!(
            self,
            write_descriptor,
            conn_id,
            handle,
            auth_req,
            value.as_ptr(),
            value.len()
        ))
    }

    pub fn execute_write(&self, conn_id: i32, execute: i32) -> BtStatus {
        BtStatus::from(ccall!(self, execute_write, conn_id, execute))
    }

    pub fn register_for_notification(
        &self,
        client_if: i32,
        addr: &RawAddress,
        handle: u16,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, register_for_notification, client_if, ffi_addr, handle))
    }

    pub fn deregister_for_notification(
        &self,
        client_if: i32,
        addr: &RawAddress,
        handle: u16,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, deregister_for_notification, client_if, ffi_addr, handle))
    }

    pub fn read_remote_rssi(&self, client_if: i32, addr: &RawAddress) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, read_remote_rssi, client_if, ffi_addr))
    }

    pub fn get_device_type(&self, addr: &RawAddress) -> i32 {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        ccall!(self, get_device_type, ffi_addr)
    }

    pub fn configure_mtu(&self, conn_id: i32, mtu: i32) -> BtStatus {
        BtStatus::from(ccall!(self, configure_mtu, conn_id, mtu))
    }

    pub fn conn_parameter_update(
        &self,
        addr: &RawAddress,
        min_interval: i32,
        max_interval: i32,
        latency: i32,
        timeout: i32,
        min_ce_len: u16,
        max_ce_len: u16,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(
            self,
            conn_parameter_update,
            ffi_addr,
            min_interval,
            max_interval,
            latency,
            timeout,
            min_ce_len,
            max_ce_len
        ))
    }

    pub fn set_preferred_phy(
        &self,
        addr: &RawAddress,
        tx_phy: u8,
        rx_phy: u8,
        phy_options: u16,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, set_preferred_phy, ffi_addr, tx_phy, rx_phy, phy_options))
    }

    // TODO(b/193916778): Figure out how to shim read_phy which accepts base::Callback

    pub fn test_command(&self, command: i32, params: &BtGattTestParams) -> BtStatus {
        BtStatus::from(ccall!(self, test_command, command, params))
    }

    pub fn get_gatt_db(&self, conn_id: i32) -> BtStatus {
        BtStatus::from(ccall!(self, get_gatt_db, conn_id))
    }
}

pub struct GattServer {
    internal: RawGattServerWrapper,
}

impl GattServer {
    pub fn register_server(&self, uuid: &Uuid, eatt_support: bool) -> BtStatus {
        BtStatus::from(ccall!(self, register_server, uuid, eatt_support))
    }

    pub fn unregister_server(&self, server_if: i32) -> BtStatus {
        BtStatus::from(ccall!(self, unregister_server, server_if))
    }

    pub fn connect(
        &self,
        server_if: i32,
        addr: &RawAddress,
        is_direct: bool,
        transport: i32,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, connect, server_if, ffi_addr, is_direct, transport))
    }

    pub fn disconnect(&self, server_if: i32, addr: &RawAddress, conn_id: i32) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, disconnect, server_if, ffi_addr, conn_id))
    }

    pub fn add_service(&self, server_if: i32, service: &[BtGattDbElement]) -> BtStatus {
        BtStatus::from(ccall!(self, add_service, server_if, service.as_ptr(), service.len()))
    }

    pub fn stop_service(&self, server_if: i32, service_handle: i32) -> BtStatus {
        BtStatus::from(ccall!(self, stop_service, server_if, service_handle))
    }

    pub fn delete_service(&self, server_if: i32, service_handle: i32) -> BtStatus {
        BtStatus::from(ccall!(self, delete_service, server_if, service_handle))
    }

    pub fn send_indication(
        &self,
        server_if: i32,
        attribute_handle: i32,
        conn_id: i32,
        confirm: i32,
        value: &[u8],
    ) -> BtStatus {
        BtStatus::from(ccall!(
            self,
            send_indication,
            server_if,
            attribute_handle,
            conn_id,
            confirm,
            value.as_ptr(),
            value.len()
        ))
    }

    pub fn send_response(
        &self,
        conn_id: i32,
        trans_id: i32,
        status: i32,
        response: &BtGattResponse,
    ) -> BtStatus {
        BtStatus::from(ccall!(self, send_response, conn_id, trans_id, status, response))
    }

    pub fn set_preferred_phy(
        &self,
        addr: &RawAddress,
        tx_phy: u8,
        rx_phy: u8,
        phy_options: u16,
    ) -> BtStatus {
        let ffi_addr = cast_to_ffi_address!(addr as *const RawAddress);
        BtStatus::from(ccall!(self, set_preferred_phy, ffi_addr, tx_phy, rx_phy, phy_options))
    }

    // TODO(b/193916778): Figure out how to shim read_phy which accepts base::Callback
}

// TODO(b/193916778): Underlying FFI is C++, implement using cxx.
pub struct BleScanner {
    _internal: RawBleScannerWrapper,
}

// TODO(b/193916778): Underlying FFI is C++, implement using cxx.
pub struct BleAdvertiser {
    _internal: RawBleAdvertiserWrapper,
}

pub struct Gatt {
    internal: RawGattWrapper,
    is_init: bool,

    pub client: GattClient,
    pub server: GattServer,
    pub scanner: BleScanner,
    pub advertiser: BleAdvertiser,

    // Keep callback object in memory (underlying code doesn't make copy)
    callbacks: Option<Box<bindings::btgatt_callbacks_t>>,
    gatt_client_callbacks: Option<Box<bindings::btgatt_client_callbacks_t>>,
    gatt_server_callbacks: Option<Box<bindings::btgatt_server_callbacks_t>>,
    gatt_scanner_callbacks: Option<Box<bindings::btgatt_scanner_callbacks_t>>,
}

impl Gatt {
    pub fn new(intf: &BluetoothInterface) -> Option<Gatt> {
        let r = intf.get_profile_interface(SupportedProfiles::Gatt);

        if r == std::ptr::null() {
            return None;
        }

        Some(Gatt {
            internal: RawGattWrapper { raw: r as *const btgatt_interface_t },
            is_init: false,
            client: GattClient {
                internal: RawGattClientWrapper {
                    raw: unsafe {
                        (*(r as *const btgatt_interface_t)).client
                            as *const btgatt_client_interface_t
                    },
                },
            },
            server: GattServer {
                internal: RawGattServerWrapper {
                    raw: unsafe {
                        (*(r as *const btgatt_interface_t)).server
                            as *const btgatt_server_interface_t
                    },
                },
            },
            scanner: BleScanner {
                _internal: RawBleScannerWrapper {
                    _raw: unsafe {
                        (*(r as *const btgatt_interface_t)).scanner as *const BleScannerInterface
                    },
                },
            },
            advertiser: BleAdvertiser {
                _internal: RawBleAdvertiserWrapper {
                    _raw: unsafe {
                        (*(r as *const btgatt_interface_t)).scanner as *const BleAdvertiserInterface
                    },
                },
            },
            callbacks: None,
            gatt_client_callbacks: None,
            gatt_server_callbacks: None,
            gatt_scanner_callbacks: None,
        })
    }

    pub fn is_initialized(&self) -> bool {
        self.is_init
    }

    pub fn initialize(
        &mut self,
        gatt_client_callbacks_dispatcher: GattClientCallbacksDispatcher,
        gatt_server_callbacks_dispatcher: GattServerCallbacksDispatcher,
    ) -> bool {
        // Register dispatcher
        if get_dispatchers()
            .lock()
            .unwrap()
            .set::<GattClientCb>(Arc::new(Mutex::new(gatt_client_callbacks_dispatcher)))
        {
            panic!("Tried to set dispatcher for GattClientCallbacks but it already existed");
        }

        if get_dispatchers()
            .lock()
            .unwrap()
            .set::<GattServerCb>(Arc::new(Mutex::new(gatt_server_callbacks_dispatcher)))
        {
            panic!("Tried to set dispatcher for GattServerCallbacks but it already existed");
        }

        let mut gatt_client_callbacks = Box::new(btgatt_client_callbacks_t {
            register_client_cb: Some(gc_register_client_cb),
            open_cb: Some(gc_open_cb),
            close_cb: Some(gc_close_cb),
            search_complete_cb: Some(gc_search_complete_cb),
            register_for_notification_cb: Some(gc_register_for_notification_cb),
            notify_cb: Some(gc_notify_cb),
            read_characteristic_cb: Some(gc_read_characteristic_cb),
            write_characteristic_cb: Some(gc_write_characteristic_cb),
            read_descriptor_cb: Some(gc_read_descriptor_cb),
            write_descriptor_cb: Some(gc_write_descriptor_cb),
            execute_write_cb: Some(gc_execute_write_cb),
            read_remote_rssi_cb: Some(gc_read_remote_rssi_cb),
            configure_mtu_cb: Some(gc_configure_mtu_cb),
            congestion_cb: Some(gc_congestion_cb),
            get_gatt_db_cb: Some(gc_get_gatt_db_cb),
            services_removed_cb: Some(gc_services_removed_cb),
            services_added_cb: Some(gc_services_added_cb),
            phy_updated_cb: Some(gc_phy_updated_cb),
            conn_updated_cb: Some(gc_conn_updated_cb),
            service_changed_cb: Some(gc_service_changed_cb),
        });

        let mut gatt_server_callbacks = Box::new(btgatt_server_callbacks_t {
            register_server_cb: Some(gs_register_server_cb),
            connection_cb: Some(gs_connection_cb),
            service_added_cb: Some(gs_service_added_cb),
            service_stopped_cb: Some(gs_service_stopped_cb),
            service_deleted_cb: Some(gs_service_deleted_cb),
            request_read_characteristic_cb: Some(gs_request_read_characteristic_cb),
            request_read_descriptor_cb: Some(gs_request_read_descriptor_cb),
            request_write_characteristic_cb: Some(gs_request_write_characteristic_cb),
            request_write_descriptor_cb: Some(gs_request_write_descriptor_cb),
            request_exec_write_cb: Some(gs_request_exec_write_cb),
            response_confirmation_cb: Some(gs_response_confirmation_cb),
            indication_sent_cb: Some(gs_indication_sent_cb),
            congestion_cb: Some(gs_congestion_cb),
            mtu_changed_cb: Some(gs_mtu_changed_cb),
            phy_updated_cb: Some(gs_phy_updated_cb),
            conn_updated_cb: Some(gs_conn_updated_cb),
        });

        let mut gatt_scanner_callbacks = Box::new(btgatt_scanner_callbacks_t {
            scan_result_cb: None,
            batchscan_reports_cb: None,
            batchscan_threshold_cb: None,
            track_adv_event_cb: None,
        });

        let mut callbacks = Box::new(btgatt_callbacks_t {
            size: 4 * 8,
            client: &mut *gatt_client_callbacks,
            server: &mut *gatt_server_callbacks,
            scanner: &mut *gatt_scanner_callbacks,
        });

        let rawcb = &mut *callbacks;

        let init = ccall!(self, init, rawcb);
        self.is_init = init == 0;
        self.callbacks = Some(callbacks);
        self.gatt_client_callbacks = Some(gatt_client_callbacks);
        self.gatt_server_callbacks = Some(gatt_server_callbacks);
        self.gatt_scanner_callbacks = Some(gatt_scanner_callbacks);

        return self.is_init;
    }
}

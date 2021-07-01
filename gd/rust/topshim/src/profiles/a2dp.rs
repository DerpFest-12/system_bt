use crate::btif::{BluetoothInterface, RawAddress};
use crate::topstack::get_dispatchers;

use num_traits::cast::FromPrimitive;
use std::sync::{Arc, Mutex};
use topshim_macros::cb_variant;

#[derive(Debug, FromPrimitive, PartialEq, PartialOrd)]
#[repr(u32)]
pub enum BtavConnectionState {
    Disconnected = 0,
    Connecting,
    Connected,
    Disconnecting,
}

impl From<u32> for BtavConnectionState {
    fn from(item: u32) -> Self {
        BtavConnectionState::from_u32(item).unwrap()
    }
}

#[derive(Debug, FromPrimitive, PartialEq, PartialOrd)]
#[repr(u32)]
pub enum BtavAudioState {
    RemoteSuspend = 0,
    Stopped,
    Started,
}

impl From<u32> for BtavAudioState {
    fn from(item: u32) -> Self {
        BtavAudioState::from_u32(item).unwrap()
    }
}

#[derive(Debug, FromPrimitive, PartialEq, PartialOrd)]
#[repr(u32)]
pub enum A2dpCodecIndex {
    SrcSbc = 0,
    SrcAac,
    SrcAptx,
    SrcAptxHD,
    SrcLdac,
    SinkSbc,
    SinkAac,
    SinkLdac,
    Max,
}

impl A2dpCodecIndex {
    pub const SRC_MIN: A2dpCodecIndex = A2dpCodecIndex::SrcSbc;
    pub const SRC_MAX: A2dpCodecIndex = A2dpCodecIndex::SinkSbc;
    pub const SINK_MIN: A2dpCodecIndex = A2dpCodecIndex::SinkSbc;
    pub const SINK_MAX: A2dpCodecIndex = A2dpCodecIndex::Max;
    pub const MAX: A2dpCodecIndex = A2dpCodecIndex::Max;
    pub const MIN: A2dpCodecIndex = A2dpCodecIndex::SrcSbc;
}

impl From<u32> for A2dpCodecIndex {
    fn from(item: u32) -> Self {
        A2dpCodecIndex::from_u32(item).unwrap_or_else(|| A2dpCodecIndex::MIN)
    }
}

#[derive(Debug, FromPrimitive, PartialEq, PartialOrd)]
#[repr(i32)]
pub enum A2dpCodecPriority {
    Disabled = -1,
    Default = 0,
    Highest = 1000_000,
}

impl From<i32> for A2dpCodecPriority {
    fn from(item: i32) -> Self {
        A2dpCodecPriority::from_i32(item).unwrap_or_else(|| A2dpCodecPriority::Default)
    }
}

bitflags! {
    pub struct A2dpCodecSampleRate: i32 {
        const RATE_NONE = 0x0;
        const RATE_44100 = 0x01;
        const RATE_48000 = 0x02;
        const RATE_88200 = 0x04;
        const RATE_96000 = 0x08;
        const RATE_176400 = 0x10;
        const RATE_192000 = 0x20;
        const RATE_16000 = 0x40;
        const RATE_24000 = 0x80;
    }
}

impl A2dpCodecSampleRate {
    pub fn validate_bits(val: i32) -> bool {
        val <= A2dpCodecSampleRate::all().bits()
    }
}

bitflags! {
    pub struct A2dpCodecBitsPerSample: i32 {
        const SAMPLE_NONE = 0x0;
        const SAMPLE_16 = 0x01;
        const SAMPLE_24 = 0x02;
        const SAMPLE_32 = 0x04;
    }
}

impl A2dpCodecBitsPerSample {
    pub fn validate_bits(val: i32) -> bool {
        val <= A2dpCodecBitsPerSample::all().bits()
    }
}

bitflags! {
    pub struct A2dpCodecChannelMode: i32 {
        const MODE_NONE = 0x0;
        const MODE_MONO = 0x01;
        const MODE_STEREO = 0x02;
    }
}

impl A2dpCodecChannelMode {
    pub fn validate_bits(val: i32) -> bool {
        val <= A2dpCodecChannelMode::all().bits()
    }
}

#[cxx::bridge(namespace = bluetooth::topshim::rust)]
pub mod ffi {
    #[derive(Debug, Copy, Clone)]
    pub struct RustRawAddress {
        address: [u8; 6],
    }

    #[derive(Debug)]
    pub struct A2dpCodecConfig {
        codec_type: i32,
        codec_priority: i32,
        sample_rate: i32,
        bits_per_sample: i32,
        channel_mode: i32,
        codec_specific_1: i64,
        codec_specific_2: i64,
        codec_specific_3: i64,
        codec_specific_4: i64,
    }

    unsafe extern "C++" {
        include!("btav/btav_shim.h");

        type A2dpIntf;

        unsafe fn GetA2dpProfile(btif: *const u8) -> UniquePtr<A2dpIntf>;

        fn init(self: Pin<&mut A2dpIntf>) -> i32;
        fn connect(self: Pin<&mut A2dpIntf>, bt_addr: RustRawAddress) -> i32;
        fn disconnect(self: Pin<&mut A2dpIntf>, bt_addr: RustRawAddress) -> i32;
        fn set_silence_device(
            self: Pin<&mut A2dpIntf>,
            bt_addr: RustRawAddress,
            silent: bool,
        ) -> i32;
        fn set_active_device(self: Pin<&mut A2dpIntf>, bt_addr: RustRawAddress) -> i32;
        fn config_codec(
            self: Pin<&mut A2dpIntf>,
            bt_addr: RustRawAddress,
            codec_preferences: Vec<A2dpCodecConfig>,
        ) -> i32;
        fn set_audio_config(self: Pin<&mut A2dpIntf>, config: A2dpCodecConfig) -> bool;
        fn start_audio_request(self: Pin<&mut A2dpIntf>) -> bool;
        fn stop_audio_request(self: Pin<&mut A2dpIntf>) -> bool;
        fn cleanup(self: Pin<&mut A2dpIntf>);

    }
    extern "Rust" {
        fn connection_state_callback(addr: RustRawAddress, state: u32);
        fn audio_state_callback(addr: RustRawAddress, state: u32);
        fn audio_config_callback(
            addr: RustRawAddress,
            codec_config: A2dpCodecConfig,
            codecs_local_capabilities: Vec<A2dpCodecConfig>,
            codecs_selectable_capabilities: Vec<A2dpCodecConfig>,
        );
        fn mandatory_codec_preferred_callback(addr: RustRawAddress);
    }
}

pub type FfiAddress = ffi::RustRawAddress;
pub type A2dpCodecConfig = ffi::A2dpCodecConfig;

impl From<RawAddress> for FfiAddress {
    fn from(addr: RawAddress) -> Self {
        FfiAddress { address: addr.val }
    }
}

impl Into<RawAddress> for FfiAddress {
    fn into(self) -> RawAddress {
        RawAddress { val: self.address }
    }
}

impl Default for A2dpCodecConfig {
    fn default() -> A2dpCodecConfig {
        A2dpCodecConfig {
            codec_type: 0,
            codec_priority: 0,
            sample_rate: 0,
            bits_per_sample: 0,
            channel_mode: 0,
            codec_specific_1: 0,
            codec_specific_2: 0,
            codec_specific_3: 0,
            codec_specific_4: 0,
        }
    }
}

#[derive(Debug)]
pub enum A2dpCallbacks {
    ConnectionState(RawAddress, BtavConnectionState),
    AudioState(RawAddress, BtavAudioState),
    AudioConfig(RawAddress, A2dpCodecConfig, Vec<A2dpCodecConfig>, Vec<A2dpCodecConfig>),
    MandatoryCodecPreferred(RawAddress),
}

pub struct A2dpCallbacksDispatcher {
    pub dispatch: Box<dyn Fn(A2dpCallbacks) + Send>,
}

type A2dpCb = Arc<Mutex<A2dpCallbacksDispatcher>>;

cb_variant!(A2dpCb, connection_state_callback -> A2dpCallbacks::ConnectionState,
FfiAddress -> RawAddress, u32 -> BtavConnectionState, {
    let _0 = _0.into();
});

cb_variant!(A2dpCb, audio_state_callback -> A2dpCallbacks::AudioState,
FfiAddress -> RawAddress, u32 -> BtavAudioState, {
    let _0 = _0.into();
});

cb_variant!(A2dpCb, mandatory_codec_preferred_callback -> A2dpCallbacks::MandatoryCodecPreferred,
FfiAddress -> RawAddress, {
    let _0 = _0.into();
});

cb_variant!(A2dpCb, audio_config_callback -> A2dpCallbacks::AudioConfig,
FfiAddress -> RawAddress, A2dpCodecConfig, Vec<A2dpCodecConfig>, Vec<A2dpCodecConfig>, {
    let _0 = _0.into();
});

pub struct A2dp {
    internal: cxx::UniquePtr<ffi::A2dpIntf>,
    _is_init: bool,
}

// For *const u8 opaque btif
unsafe impl Send for A2dp {}

impl A2dp {
    pub fn new(intf: &BluetoothInterface) -> A2dp {
        let a2dpif: cxx::UniquePtr<ffi::A2dpIntf>;
        unsafe {
            a2dpif = ffi::GetA2dpProfile(intf.as_raw_ptr());
        }

        A2dp { internal: a2dpif, _is_init: false }
    }

    pub fn initialize(&mut self, callbacks: A2dpCallbacksDispatcher) -> bool {
        if get_dispatchers().lock().unwrap().set::<A2dpCb>(Arc::new(Mutex::new(callbacks))) {
            panic!("Tried to set dispatcher for A2dp callbacks while it already exists");
        }
        self.internal.pin_mut().init();
        true
    }

    pub fn connect(&mut self, device: String) {
        let addr = RawAddress::from_string(device.clone());
        if addr.is_none() {
            eprintln!("Invalid device string {}", device);
            return;
        }
        self.internal.pin_mut().connect(addr.unwrap().into());
    }

    pub fn set_active_device(&mut self, device: String) {
        let addr = RawAddress::from_string(device.clone());
        if addr.is_none() {
            eprintln!("Invalid device string {}", device);
            return;
        }
        self.internal.pin_mut().set_active_device(addr.unwrap().into());
    }

    pub fn disconnect(&mut self, device: String) {
        let addr = RawAddress::from_string(device.clone());
        if addr.is_none() {
            eprintln!("Invalid device string {}", device);
            return;
        }
        self.internal.pin_mut().disconnect(addr.unwrap().into());
    }

    pub fn set_audio_config(&mut self, sample_rate: i32, bits_per_sample: i32, channel_mode: i32) {
        let config = A2dpCodecConfig {
            sample_rate: sample_rate,
            bits_per_sample: bits_per_sample,
            channel_mode: channel_mode,
            ..Default::default()
        };
        self.internal.pin_mut().set_audio_config(config);
    }
    pub fn start_audio_request(&mut self) {
        self.internal.pin_mut().start_audio_request();
    }

    pub fn stop_audio_request(&mut self) {
        self.internal.pin_mut().stop_audio_request();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validate_sample_rate() {
        assert!(!A2dpCodecSampleRate::validate_bits(256));
        assert!(A2dpCodecSampleRate::validate_bits(2 + 32 + 128));
    }

    #[test]
    fn validate_bits_per_sample() {
        assert!(!A2dpCodecBitsPerSample::validate_bits(8));
        assert!(A2dpCodecBitsPerSample::validate_bits(1 + 4));
    }

    #[test]
    fn validate_channel_mode() {
        assert!(!A2dpCodecChannelMode::validate_bits(4));
        assert!(A2dpCodecChannelMode::validate_bits(1 + 2));
    }
}

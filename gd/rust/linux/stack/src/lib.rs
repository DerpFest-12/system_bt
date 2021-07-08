//! Fluoride/GD Bluetooth stack.
//!
//! This crate provides the API implementation of the Fluoride/GD Bluetooth stack, independent of
//! any RPC projection.

#[macro_use]
extern crate num_derive;

pub mod bluetooth;
pub mod bluetooth_gatt;
pub mod bluetooth_media;

use bt_topshim::btif::BaseCallbacks;
use bt_topshim::profiles::a2dp::A2dpCallbacks;

use std::sync::{Arc, Mutex};

use tokio::sync::mpsc::channel;
use tokio::sync::mpsc::{Receiver, Sender};

use crate::bluetooth::Bluetooth;
use crate::bluetooth_media::BluetoothMedia;

/// Represents a Bluetooth address.
// TODO: Add support for LE random addresses.

/// Message types that are sent to the stack main dispatch loop.
pub enum Message {
    A2dp(A2dpCallbacks),
    Base(BaseCallbacks),
    BluetoothCallbackDisconnected(u32),
}

/// Umbrella class for the Bluetooth stack.
pub struct Stack {}

impl Stack {
    /// Creates an mpsc channel for passing messages to the main dispatch loop.
    pub fn create_channel() -> (Sender<Message>, Receiver<Message>) {
        channel::<Message>(1)
    }

    /// Runs the main dispatch loop.
    pub async fn dispatch(
        mut rx: Receiver<Message>,
        bluetooth: Arc<Mutex<Box<Bluetooth>>>,
        bluetooth_media: Arc<Mutex<Box<BluetoothMedia>>>,
    ) {
        loop {
            let m = rx.recv().await;

            if m.is_none() {
                eprintln!("Message dispatch loop quit");
                break;
            }

            match m.unwrap() {
                Message::A2dp(a) => {
                    bluetooth_media.lock().unwrap().dispatch_a2dp_callbacks(a);
                }
                Message::Base(b) => {
                    bluetooth.lock().unwrap().dispatch_base_callbacks(b);
                }

                Message::BluetoothCallbackDisconnected(id) => {
                    bluetooth.lock().unwrap().callback_disconnected(id);
                }
            }
        }
    }
}

/// Signifies that the object may be a proxy to a remote RPC object.
///
/// An object that implements RPCProxy trait signifies that the object may be a proxy to a remote
/// RPC object. Therefore the object may be disconnected and thus should implement
/// `register_disconnect` to let others observe the disconnection event.
pub trait RPCProxy {
    /// Registers disconnect observer that will be notified when the remote object is disconnected.
    fn register_disconnect(&mut self, f: Box<dyn Fn() + Send>);

    /// Returns the ID of the object. For example this would be an object path in D-Bus RPC.
    fn get_object_id(&self) -> String;
}

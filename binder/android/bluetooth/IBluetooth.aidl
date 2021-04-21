/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.bluetooth;

import android.bluetooth.IBluetoothCallback;
import android.bluetooth.IBluetoothConnectionCallback;
import android.bluetooth.IBluetoothMetadataListener;
import android.bluetooth.IBluetoothOobDataCallback;
import android.bluetooth.IBluetoothSocketManager;
import android.bluetooth.IBluetoothStateChangeCallback;
import android.bluetooth.BluetoothActivityEnergyInfo;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.OobData;
import android.content.AttributionSource;
import android.os.ParcelUuid;
import android.os.ParcelFileDescriptor;
import android.os.ResultReceiver;

/**
 * System private API for talking with the Bluetooth service.
 *
 * {@hide}
 */
interface IBluetooth
{
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    int getState();

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean enable(boolean quietMode);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean disable();

    @UnsupportedAppUsage
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(allOf={android.Manifest.permission.BLUETOOTH_CONNECT,android.Manifest.permission.LOCAL_MAC_ADDRESS})")
    String getAddress();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    ParcelUuid[] getUuids();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean setName(in String name);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    String getName();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_ADVERTISE)")
    int getNameLengthForAdvertise();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    BluetoothClass getBluetoothClass();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setBluetoothClass(in BluetoothClass bluetoothClass);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getIoCapability();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setIoCapability(int capability);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getLeIoCapability();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setLeIoCapability(int capability);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    int getScanMode();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    boolean setScanMode(int mode, int duration);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    int getDiscoverableTimeout();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    boolean setDiscoverableTimeout(int timeout);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    boolean startDiscovery(in AttributionSource attributionSource);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    boolean cancelDiscovery();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_SCAN)")
    boolean isDiscovering();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    long getDiscoveryEndMillis();

    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    int getAdapterConnectionState();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    int getProfileConnectionState(int profile);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    BluetoothDevice[] getBondedDevices();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean createBond(in BluetoothDevice device, in int transport, in OobData p192Data,
    in OobData p256Data, String packageName);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean cancelBondProcess(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean removeBond(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getBondState(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean isBondingInitiatedLocally(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    long getSupportedProfiles();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getConnectionState(in BluetoothDevice device);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    String getRemoteName(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getRemoteType(in BluetoothDevice device);
    @UnsupportedAppUsage
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    String getRemoteAlias(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean setRemoteAlias(in BluetoothDevice device, in String name, in String callingPackage);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getRemoteClass(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    ParcelUuid[] getRemoteUuids(in BluetoothDevice device);
    @UnsupportedAppUsage
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean fetchRemoteUuids(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean sdpSearch(in BluetoothDevice device, in ParcelUuid uuid);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getBatteryLevel(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getMaxConnectedAudioDevices();

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean setPin(in BluetoothDevice device, boolean accept, int len, in byte[] pinCode);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    boolean setPasskey(in BluetoothDevice device, boolean accept, int len, in byte[]
    passkey);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setPairingConfirmation(in BluetoothDevice device, boolean accept);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getPhonebookAccessPermission(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setSilenceMode(in BluetoothDevice device, boolean silence);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean getSilenceMode(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setPhonebookAccessPermission(in BluetoothDevice device, int value);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getMessageAccessPermission(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setMessageAccessPermission(in BluetoothDevice device, int value);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    int getSimAccessPermission(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setSimAccessPermission(in BluetoothDevice device, int value);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    void registerCallback(in IBluetoothCallback callback);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    void unregisterCallback(in IBluetoothCallback callback);

    // For Socket
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    IBluetoothSocketManager getSocketManager();

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean factoryReset();

    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isMultiAdvertisementSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isOffloadedFilteringSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isOffloadedScanBatchingSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isActivityAndEnergyReportingSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isLe2MPhySupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isLeCodedPhySupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isLeExtendedAdvertisingSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    boolean isLePeriodicAdvertisingSupported();
    @JavaPassthrough(annotation="@android.annotation.RequiresNoPermission")
    int getLeMaximumAdvertisingDataLength();

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    BluetoothActivityEnergyInfo reportActivityInfo();

    // For Metadata
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean registerMetadataListener(in IBluetoothMetadataListener listener, in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean unregisterMetadataListener(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean setMetadata(in BluetoothDevice device, in int key, in byte[] value);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    byte[] getMetadata(in BluetoothDevice device, in int key);

    /**
     * Requests the controller activity info asynchronously.
     * The implementor is expected to reply with the
     * {@link android.bluetooth.BluetoothActivityEnergyInfo} object placed into the Bundle with the
     * key {@link android.os.BatteryStats#RESULT_RECEIVER_CONTROLLER_KEY}.
     * The result code is ignored.
     */
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    oneway void requestActivityInfo(in ResultReceiver result);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    void onLeServiceUp();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    void onBrEdrDown();

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(allOf={android.Manifest.permission.BLUETOOTH_CONNECT,android.Manifest.permission.BLUETOOTH_PRIVILEGED,android.Manifest.permission.MODIFY_PHONE_STATE})")
    boolean connectAllEnabledProfiles(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(allOf={android.Manifest.permission.BLUETOOTH_CONNECT,android.Manifest.permission.BLUETOOTH_PRIVILEGED})")
    boolean disconnectAllEnabledProfiles(in BluetoothDevice device);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(allOf={android.Manifest.permission.BLUETOOTH_CONNECT,android.Manifest.permission.BLUETOOTH_PRIVILEGED,android.Manifest.permission.MODIFY_PHONE_STATE})")
    boolean setActiveDevice(in BluetoothDevice device, in int profiles);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)")
    List<BluetoothDevice> getMostRecentlyConnectedDevices();
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(allOf={android.Manifest.permission.BLUETOOTH_CONNECT,android.Manifest.permission.BLUETOOTH_PRIVILEGED,android.Manifest.permission.MODIFY_PHONE_STATE})")
    boolean removeActiveDevice(in int profiles);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean registerBluetoothConnectionCallback(in IBluetoothConnectionCallback callback);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean unregisterBluetoothConnectionCallback(in IBluetoothConnectionCallback callback);

    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    boolean canBondWithoutDialog(in BluetoothDevice device);
    @JavaPassthrough(annotation="@android.annotation.RequiresPermission(android.Manifest.permission.BLUETOOTH_PRIVILEGED)")
    void generateLocalOobData(in int transport, IBluetoothOobDataCallback callback);
}

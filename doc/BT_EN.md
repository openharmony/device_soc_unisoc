### BT Framework

The OpenHarmony Bluetooth architecture is illustrated in the following diagram:

![bt构架.jpg](figures/bt构架.jpg)

APP: The Bluetooth application program, which implements application functionality by calling Bluetooth interfaces (interface).

Bluetooth: The Bluetooth framework layer, primarily consisting of the interface and services. The interface is responsible for providing functional APIs to upper-layer applications. The services are responsible for implementing the interface APIs and the Bluetooth protocol stack.

Vendor: The z.so library compiled from vendor-provided code, containing the OpenHarmony-defined vendor interface and the vendor's proprietary vendor lib.

HardWare: The Bluetooth hardware device, which refers to the Bluetooth controller (controler).

The overall Bluetooth hardware architecture is divided into two parts: the host and the controller. Communication between the host and controller follows the Host Controller Interface (HCI), as shown below:

![BT-HCI.jpg](figures/BT-HCI.jpg)

HCI (Host Controller Interface) defines how to exchange commands, events, asynchronous (ACL), and synchronous (SCO) data packets. Asynchronous packets (ACL) are used for data transmission, while synchronous packets (SCO) are used for voice with headset and hands-free profiles.

The adaptation work primarily involves completing the HCI interface integration.

### Hardware Connection

The Unisoc 7885 SoC integrates the Bluetooth controller internally. Therefore, the HCI is not a physical UART, SDIO, or USB interface but a virtual channel.

### OpenHarmony 7885 Bluetooth Adaptation

As Bluetooth is integrated within the Unisoc 7885, the vendor-provided HAL layer code is modified and compiled into libbt_vendor.z.so for use by the BT HDI.

 

The libbt_vendor.z.so library must hook up and implement the following three interface components:

1. The bt_vendor_interface_t structure defines the external interface of the vendor lib, which includes three functions: init, op, and close.

```c++
  typedef struct {
     /**
      * Set to sizeof(bt_vndor_interface_t)
      */
     size_t size;

     /**
      * Caller will open the interface and pass in the callback routines
      * to the implemenation of this interface.
      */
     int (*init)(const bt_vendor_callbacks_t* p_cb, unsigned char* local_bdaddr);

     /**
      * Vendor specific operations
      */
     int (*op)(bt_opcode_t opcode, void* param);

     /**
      * Closes the interface
      */
     void (*close)(void);
 } bt_vendor_interface_t;
```

 

The bt_vendor_interface_t must be defined within the vendor lib. Below is reference code:

```c++
// vendor lib接口定义
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
   sizeof(bt_vendor_interface_t),
   init,
   op,
   cleanup };
```

2. bt_vendor_callbacks_t contains hook functions provided by OpenHarmony for the vendor lib to call. The bt_vendor_callbacks_t structure must be passed to the vendor lib when calling the bt_vendor_interface_t.init function.

**

```c++
/**
 * initialization callback.
 */
typedef void (*init_callback)(bt_op_result_t result);

/** 
 * call the callback to malloc a size of buf.
 */
typedef void* (*malloc_callback)(int size);

/**
 * call the callback to free buf
 */
typedef void (*free_callback)(void* buf);

/**
 *  hci command packet transmit callback
 *  Vendor lib calls cmd_xmit_cb function in order to send a HCI Command
 *  packet to BT Controller. 
 *
 *  The opcode parameter gives the HCI OpCode (combination of OGF and OCF) of
 *  HCI Command packet. For example, opcode = 0x0c03 for the HCI_RESET command
 *  packet.
 */
typedef size_t (*cmd_xmit_callback)(uint16_t opcode, void* p_buf);

typedef struct {
    /**
     * set to sizeof(bt_vendor_callbacks_t)
     */
    size_t size;

    /* notifies caller result of init request */
    init_callback init_cb;

    /* buffer allocation request */
    malloc_callback alloc;

    /* buffer free request */
    free_callback dealloc;

    /* hci command packet transmit request */
    cmd_xmit_callback xmit_cb;
} bt_vendor_callbacks_t;
```

 

init_cb: Used to notify the configuration result.

alloc: Used to request memory allocation.

dealloc: Used to request memory deallocation.

xmit_cb: Used to send HCI commands.

The definitions of these hook functions in OpenHarmony:

 

```c++
// vendor_interface.cpp

bt_vendor_callbacks_t VendorInterface::vendorCallbacks_ = {
    .size = sizeof(bt_vendor_callbacks_t),
    .init_cb = VendorInterface::OnInitCallback,
    .alloc = VendorInterface::OnMallocCallback,
    .dealloc = VendorInterface::OnFreeCallback,
    .xmit_cb = VendorInterface::OnCmdXmitCallback,
};
```

3. Implementation of the op function for various operation codes.

The op function must handle the following op codes accordingly.

```c++
/**
 * BT vendor lib cmd.
 */
typedef enum {
    /**
     * Power on the BT Controller.
     * @return 0 if success.
     */
    BT_OP_POWER_ON,

    /**
     * Power off the BT Controller.
     * @return 0 if success.
     */
    BT_OP_POWER_OFF,

    /**
     * Establish hci channels. it will be called after BT_OP_POWER_ON.
     * @param int (*)[HCI_MAX_CHANNEL].
     * @return fd count.
     */
    BT_OP_HCI_CHANNEL_OPEN,

    /**
     * Close all the hci channels which is opened.
     */
    BT_OP_HCI_CHANNEL_CLOSE,

    /**
     * initialization the BT Controller. it will be called after BT_OP_HCI_CHANNEL_OPEN.
     * Controller Must call init_cb to notify the host once it has been done.
     */
    BT_OP_INIT,

    /**
     * Get the LPM idle timeout in milliseconds.
     * @param (uint_32 *)milliseconds, btc will return the value of lpm timer.
     * @return 0 if success.
     */
    BT_OP_GET_LPM_TIMER,

    /**
     * Enable LPM mode on BT Controller.
     */
    BT_OP_LPM_ENABLE,

    /**
     * Disable LPM mode on BT Controller.
     */
    BT_OP_LPM_DISABLE,

    /**
     * Wakeup lock the BTC.
     */
    BT_OP_WAKEUP_LOCK,

    /**
     * Wakeup unlock the BTC.
     */
    BT_OP_WAKEUP_UNLOCK,

    /**
     * transmit event response to vendor lib.
     * @param (void *)buf, struct of HC_BT_HDR.
     */
    BT_OP_EVENT_CALLBACK
} bt_opcode_t;
```


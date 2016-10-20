# chapter 4 RDMA

------

### remote direct memory access

## 1.Traditional  DMA

A method that IO exchange is  totally exectued by hardware.DMA controller  take the control of bus from cpu completely and exchange data between IO devices and memory. When it works,DMA emits the address and control signal to the memory to update the address and count the number of word ,then report it to cpu after it's done.

**[Advantage]**:

1. Operations implemented by hardware completely

2. High speed

3. Nearly not interrupt cpu, IO devices work with cpu parallel

   ​

##2.Principle of RDMA


​    ![RDMA工作体系结构](C:\Users\BlackWall\Desktop\RDMA工作体系结构.jpg)

   **[Progress]**

1.    The application that runs in user space produces a RDMA request to local NIC
2.    NIC will read the buffer and transmit the content to remote NIC
3.    RDMA informations transmitted on the network contain virtual target address,memory key                and the data.After the completion of request,it can be processed in user space totally or be              processed by kernel memory when the application sleep over the completion.        
## 3.Zero copy tec

​     This tec makes it possible that NIC directly exchange data with application memory,and this  eliminates necessity of copying data between application memory and kernel memory.Kernel memory bypass let the app send instructions to NIC without calling kernel memory,which reduces the  context switch between kernel memory space and user space.

 ![RDMA中的零拷贝技术](C:\Users\BlackWall\Desktop\RDMA中的零拷贝技术.jpg)

 Actually, the data to send or receive is cached in a tagged storage space.And this space can be mapped into application space by the rule negotiated by the LLP and RDMA,which eliminates the least twice copy in tradition.The thin line indicates direction of dataflow.

## 4. Consist of RDMA

​      ![RDMA的构成](C:\Users\BlackWall\Desktop\RDMA的构成.jpg)

  The RDMA is implemented by cooperation of RDMA,DDP and MPA3,which consist the family of iWARP protocol to ensure the interoperation of highspeed network. 

  RDMA Layer:transform the RDMA read ,write and send message into RDMA "message" that is equivalent to the message of application layer in TCP/IP.And DDP break the message into segments of DDP datagram .Then repost them to MPA to add marker ,length and CRC.

## 5.RDMA Operation

   RDMA operations start by pinning memory.When you pin memroy you are telling the kernel that this memory is owned by the application.So it will not be swapped out.(The registerations are stored in memory translation and protection tables which can be stored in system or adapter memory.These registrations are then used to translate and validate data that is being transmitted from or placed into registered buffer.)Then we tell the HCA to address  the memory and prepare a channel from the card to the memory. We refer to this as registering a Memory Region that can be used to perform any RDMA operations.

 ![qp-basic1](C:\Users\BlackWall\Desktop\quick concepts\qp-basic1.jpg)

RDMA communication is based on a set of three queues.The send queue and receive queue are responsible for scheduling work.They are always created in pairs.So they are referred to as a **Queue Pair(QP)**.A completion queue is used to notify us when the instructions placed on the work queues have been completed. A user places instructions on it’s work queues that tells the HCA what buffers it wants to send or receive. These instructions are small structs called work requests or **Work Queue Elements (WQE)**. WQE is pronounced “WOOKIE” like the creature from starwars. A WQE primarily contains a pointer to a buffer. A WQE placed on the send queue contains a pointer to the message to be sent. A pointer in the WQE on the receive queue contains a pointer to a buffer where an incoming message from the wire can be placed.

RDMA is an asynchronous transport mechanism. So we can queue a number of send or receive WQEs at a time. The HCA will process these WQE in order as fast as it can. When the WQE is processed the data is moved. Once the transaction completes a **Completion Queue Element (CQE)** is created and placed on the **Completion Queue (CQ).** We call a CQE a “COOKIE”.

To help folks map existing APIs over RDMA Model ,we observed the following:
\* The layer between the API and the RDMA Model should support **memory registration** and two forms of data transfer, **Send/Receive and RDMA Write/Read.**
\* **Long lived memory registrations** are used for Send/Receive data transfers. That is, the RDMA library performs a user space copy of the user's buffer into/out-of a long lived registered buffer.
\* **Dynamic memory registrations** are used for RDMA Write/Read data transfers. That is, if the user space buffer isn't already registered (see 7th bullet in this list), the RDMA library dynamically registers the user space buffer.
\* Memory registration has a finite number of deterministic CPU instructions, consisting of a context switching into the kernel, memory pinning by the kernel, memory registration with the adapter and a context switch back to user space.
\* Out of user space memory copies also have a finite number of deterministic CPU instructions.
\* If the overhead associated with buffer registration and de-registration exceeds the overhead associated with a user space buffer copy, then Send/Receive should be used. Otherwise RDMA Write/Read should be used. That is, **Send/Receive** should be used for **control and small** **messages**; and **RDMA Writes/Reads for larger messages**, where the cut-off between small and large is depends on the overheads (registration vs copy) associated with the OS/processor platform.
\* Additionally, **lazy de-registration** algorithms were created to avoid the registration overhead for the case where the programmer **re-uses buffers**.
\* For RDMA Writes and Reads, a buffer advertisement mechanism is necessary. The mechanism can be a-priori or a part of the programming model's wire protocol.
\* The cost of scaling a reliable RDMA transport (IB Reliable Connected or iWARP) is analogous to the cost of scaling a reliable non-RDMA transport (e.g. TCP). That is, each connection must retain a context that is used for reliability checks. If reliability is required, then performing the reliability checks are required, whether they be performed in the host CPU or the adapter. If they are performed in the adapter, then to scale efficiently the adapter must have high bandwidth access to the context, just like the host CPU must have high bandwidth access to the context if the reliability check is performed on the host.


## 6.Sample

Example:move a buffer from A to B(memory)   also call it Message Passing semantics

step1: A: identify a buffer to tranfer      B:create an empty buffer allocated for the data to be placed

 ![rdma-send-example-step-11](C:\Users\BlackWall\Desktop\quick concepts\rdma-send-example-step-11.jpg)

step2:System B creates a WQE “WOOKIE” and places in on the Receive Queue. This WQE contains a pointer to the memory buffer where the data will be placed. System A also creates a WQE which points to the buffer in it’s memory that will be transmitted. ![rdma-send-example-step-2](C:\Users\BlackWall\Desktop\quick concepts\rdma-send-example-step-2.jpg)

Step 3 The HCA is always working in hardware looking for WQE’s on the send queue. The HCA will consume the WQE from System A and begin streaming the data from the memory region to system B.  When data begins arriving at System B the HCA will consume the WQE in the receive queue to learn where it should place the data. The data streams over a highspeed channel bypassing the kernel. ![rdma-send-example-step-3](C:\Users\BlackWall\Desktop\quick concepts\rdma-send-example-step-3.jpg)

Step 4 When the data movement completes the HCA will create a CQE “COOKIE”. This is placed in the Completion Queue and indicates that the transaction has completed. For every WQE consumed a CQE is generated. So a CQE is created on System A ‘s CQ indicating that the operation completed and also on System B’s CQ. A CQE is always generated even if there was an error. The CQE will contain field indicating the status of the transaction. ![rdma-send-example-step-4](C:\Users\BlackWall\Desktop\quick concepts\rdma-send-example-step-4.jpg)

## 7.Details of establishing connections

[Passive Side]:

\*Create an event channel
\*Bind to an address
\*Create a listener and return the port/address
\*Wait for a connection request
\*Create a PD,CQ,QP
\*Accept the connection request
\*Wait for the connection to be established
\*Post operations as appropriate

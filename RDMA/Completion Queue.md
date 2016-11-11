#   Completion Queue

## ibv_create_cq()

When an outstanding Work Request, within a Send or Receive Queue, is completed, a Work Completion is being added to the CQ of that Work Queue. This Work Completion indicates that the outstanding Work Request has been completed (and no longer considered outstanding) and provides details on it (status, direction, opcode, etc.).

A single CQ can be shared for sending, receiving, and sharing across multiple QPs. The Work Completion holds the information to specify the QP number and the Queue (Send or Receive) that it came from.

The user can define the minimum size of the CQ. The actual created size can be equal or higher than this value.

#### What will happen if the CQ size that I choose is too small??

If there will be a case that the CQ will be full and a new Work Completion is going to be added to that CQ, there will be a CQ overrun. A Completion with error will be added to the CQ and an asynchronous event **IBV_EVENT_CQ_ERR** will be generated.

## ibv_resize_cq()

If the CQ size is being decreased, there are some limitations to the new size:

- It can't be less than the number of Work Completion that currently exists in the CQ
- It should be enough to hold the Work Completions that may enter this CQ (otherwise, CQ overrun will occur(
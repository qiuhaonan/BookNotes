# SRQ

## ibv_create_srq()

**ibv_create_srq()** creates a Shared Receive Queue (SRQ) associated with a Protection Domain.

This SRQ will later be used when calling **ibv_create_qp()** to indicate that the Receive Queue of that Queue Pair is shared with other Queue Pairs.

#### Which attributes should be used for an SRQ?

The number of Work Requests should be enough to hold Receive Requests for all of the incoming Messages in all of the QPs that are associated with this SRQ.
The number of scatter/gather elements should be the maximum number of scatter/gather elements in any Work Request that will be posted to this SRQ.
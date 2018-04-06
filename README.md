
[Almost] Standalone Map/Reduce. Works on top of PUB/SUB engine
Work in progress: adaptation to be plugable into Hadoop and [maybe] Spark

#### Lightweight MapReduce 

The MapReduce programming model consistently gained ground as a viable solution for assuring the necessary scalability of distributed data processing.
The generic model, composed of the two main *map* and *reduce* functions, was widely used to implement applications that can leverage parallel task processing.
In this generic model, the data to be processed is typically located over a series of mapper nodes, which apply in parallel a *map* function responsible for mapping individual data items to a finite set of predefined keys.
The output is redistributed in a shuffle step based on the key mappings to reducer nodes, which execute in parallel a *reduce* function for processing each set of data items corresponding to a certain key.
This model can be used in processing tasks that range from simple operations that can be composed like counting, sorting, and searching data to more complex algorithms like cross-correlation or page rank.

Package dependencies:
* libzmq3-dev
* libboost-system-dev
* libboost-chrono-dev
* libcrypto++-dev

In such a system, we have two roles:
* Client : sends code and data to workers, receives the output
* Worker : responsible for processing data based on received code

Workers are autonomous, they should be accordingly started.
The system interface lies on the client side.
Since it reuses SCBR as dissemination channel, SCBR message and communication interfaces also apply here.
However, the Lightweight MapReduce framework has a protocol of its own.
Refer to the paper to get more information.


[More info](https://arxiv.org/abs/1705.05684)

If the three repositories bellow are cloned in a common root, relative paths should already be correctly configured. Otherwise, fixes must be made in the Makefiles.

**Source code:**
```
$ git clone https://gitlab.securecloud.works/rafael.pires/mapreduce.git
$ git clone https://gitlab.securecloud.works/rafael.pires/scbr.git
```

**Dependency:**
```
$ git clone https://gitlab.securecloud.works/rafael.pires/sgx_common.git

```

##### Message interface

Within the SCBR publication header we use the following fields:
* **sid** : Session identification. It can be a random number used to identify each session, so that messages belonging to distinct ones are not mixed.
* **type** : it tags each publication according to its role in the protocol, as follows:
  * `INVALID_TYPE` : undefined message
  * `MAP_CODETYPE` : the publication conveys code regarding the map step
  * `REDUCE_CODETYPE` : the publication conveys code regarding the reduce step
  * `MAP_DATATYPE` : the publication conveys data to be processed by mappers
  * `REDUCE_DATATYPE` : the publication conveys data to be processed by reducers
  * `RESULT_DATATYPE` : the publication conveys resulting data after processing
  * `JOB_ADVERTISE` : the publication anounces that a new task is available 
  * `JOB_REQUEST` : the publication conveys data that allows a worker to be assigned to a given task

The session identification (`sid`) field and type of message (`type`) in which the subscriber is interested are also used in the filter. Besides, subscription headers also include the node identification. All three fields use equality operator, meaning that all three fields need to match a publication header.
* **dst** : identification of a node intended to receive messages belonging to a tipe and session.

__Input:__ sender identification, vector of header's keys and values

__Output:__ SCBR headers

__Users:__ clients

##### Provisioning of code and data

__Input:__ filenames for code (in Lua scripting language) and data

__Output:__ none. It dispatches jobs

__Users:__ clients


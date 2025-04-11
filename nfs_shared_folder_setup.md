
##  NFS Shared Folder Setup for MPI Cluster

This section describes how to configure a shared folder using **NFS (Network File System)** so all nodes in the MPI cluster can access the same working directory.

---

###  Master Node (NFS Server)

#### 1. Install NFS Server

```bash
sudo apt update
sudo apt install nfs-kernel-server
```

#### 2. Create and Set Permissions for the Shared Folder

```bash
mkdir -p /home/master/Desktop/labs
chmod 755 /home/master/Desktop/labs
```

#### 3. Edit the Exports File

```bash
sudo nano /etc/exports
```

Add the following line:

```
/home/master/Desktop/labs 192.168.1.0/24(rw,sync,no_subtree_check)
```

> Replace `192.168.1.0/24` with your actual subnet (or use specific IPs).

#### 4. Apply and Restart NFS Service

```bash
sudo exportfs -a
sudo systemctl restart nfs-kernel-server
```

---

###  Slave Nodes (NFS Clients)

#### 1. Install NFS Client

```bash
sudo apt update
sudo apt install nfs-common
```

#### 2. Create the Mount Point (Same Path as Master)

```bash
mkdir -p /home/master/Desktop/labs
```

#### 3. Mount the Shared Folder

```bash
sudo mount 192.168.1.10:/home/master/Desktop/labs /home/master/Desktop/labs
```

> Replace `192.168.1.10` with the master node’s IP address.

---

###  (Optional) Auto-Mount on Boot

To automatically mount the shared folder at startup, edit `/etc/fstab`:

```bash
sudo nano /etc/fstab
```

Add this line:

```
192.168.1.10:/home/master/Desktop/labs /home/master/Desktop/labs nfs defaults 0 0
```

---

###  Test It

On the master node:

```bash
touch /home/master/Desktop/labs/testfile.txt
```

On the slave:

```bash
ls /home/master/Desktop/labs
```

You should see `testfile.txt`.

---

Now the MPI nodes share the same directory and you no longer need to copy executables manually. 

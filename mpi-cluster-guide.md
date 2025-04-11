# MPI Cluster Setup using VirtualBox and LAM/MPI

This guide explains how to set up a simple MPI cluster using VirtualBox and LAM/MPI. The cluster includes one master and three slave nodes, communicating via SSH. We use Ubuntu Server for all nodes. The final goal is to run matrix addition and multiplication using MPI `scatter`, `broadcast`, and `gather` operations.

---

## Table of Contents
- [Requirements](#requirements)
- [Step 1: Install VirtualBox and Ubuntu Server](#step-1-install-virtualbox-and-ubuntu-server)
- [Step 2: Create Virtual Machines](#step-2-create-virtual-machines)
- [Step 3: Networking Configuration](#step-3-networking-configuration)
- [Step 4: Ubuntu Server Installation](#step-4-ubuntu-server-installation)
- [Step 5: Configure Network for Node Communication](#step-5-configure-network-for-node-communication)
- [Step 6: Installing LAM/MPI and Dependencies](#step-6-installing-lammpi-and-dependencies)
- [Step 7: SSH Key-Based Authentication](#step-7-ssh-key-based-authentication)
- [Step 8: Configure LAM and Boot the Cluster](#step-8-configure-lam-and-boot-the-cluster)
- [Step 9: Creating MPI Programs](#step-9-creating-mpi-programs)
- [Step 10: Compile and Distribute MPI Programs](#step-10-compile-and-distribute-mpi-programs)
- [Step 11: Running MPI Programs](#step-11-running-mpi-programs)
- [Step 12: Troubleshooting](#step-12-troubleshooting)

---

## Requirements
- Oracle VirtualBox (Installed on host machine)
- Ubuntu Server ISO (e.g., Ubuntu Server 22.04 LTS)
- Host system with at least:
  - 16 GB RAM
  - 45+ GB free disk space

---

## Step 1: Install VirtualBox and Ubuntu Server
- Download VirtualBox from https://www.virtualbox.org/
- Download Ubuntu Server ISO from https://ubuntu.com/download/server

---

## Step 2: Create Virtual Machines
Create four VMs:
- **master** (hostname: `master`, IP: `192.168.1.10`)
- **slave1** (hostname: `slave1`, IP: `192.168.1.11`)
- **slave2** (hostname: `slave2`, IP: `192.168.1.12`)
- **slave3** (hostname: `slave3`, IP: `192.168.1.13`)

VM Settings:
- 2 GB RAM each (4 GB for master if using GUI)
- 10–15 GB disk space
- **Network: Two Network Adapters:**
  - Adapter 1: NAT (for internet access)
  - Adapter 2: Internal Network (for inter-VM communication)

---

## Step 3: Networking Configuration
For each VM, edit the netplan configuration file:

```bash
sudo nano /etc/netplan/00-installer-config.yaml
```

Use this configuration (adjust based on your interface names):
```yaml
network:
  version: 2
  ethernets:
    enp0s3:  # First adapter (NAT) - for internet access
      dhcp4: yes
    enp0s8:  # Second adapter (Internal) - for cluster communication
      dhcp4: no
      addresses: [192.168.1.10/24]  # Use appropriate IP for each node
```

Apply the configuration:
```bash
sudo chmod 600 /etc/netplan/00-installer-config.yaml  # Fix permissions
sudo netplan apply
```

**Note:** Make sure the YAML indentation is correct (use spaces, not tabs).

Edit `/etc/hosts` on each VM:
```
127.0.0.1 localhost
192.168.1.10 master
192.168.1.11 slave1
192.168.1.12 slave2
192.168.1.13 slave3
```

Set hostname on each VM:
```bash
sudo hostnamectl set-hostname master  # Or slave1, slave2, slave3
```

---

## Step 4: Ubuntu Server Installation
- Boot each VM with the Ubuntu Server ISO
- During install, create users with consistent usernames across all VMs
- Install OpenSSH server when prompted during installation

---

## Step 5: Configure Network for Node Communication
Verify connectivity between nodes:
```bash
ping -c 3 master
ping -c 3 slave1
ping -c 3 slave2
ping -c 3 slave3
```

If pinging fails, troubleshoot:
1. Check if the netplan configuration was applied correctly
2. Verify internal network settings in VirtualBox
3. Check firewall settings:
   ```bash
   sudo ufw status
   # If active, allow internal traffic
   sudo ufw allow from 192.168.1.0/24
   # Or temporarily disable
   sudo ufw disable
   ```

---

## Step 6: Installing LAM/MPI and Dependencies
Install the following on **all VMs**:
```bash
sudo apt update
sudo apt install -y build-essential openmpi-bin openmpi-common libopenmpi-dev lam-runtime lam-dev
```

---

## Step 7: SSH Key-Based Authentication
On **master only**:
```bash
ssh-keygen -t rsa
# Press Enter for all prompts to use defaults
```

Copy the key to each slave:
```bash
ssh-copy-id username@slave1
ssh-copy-id username@slave2
ssh-copy-id username@slave3
```

Test connections:
```bash
ssh slave1 hostname
ssh slave2 hostname
ssh slave3 hostname
```

Each command should return the hostname without password prompts.

---

## Step 8: Configure LAM and Boot the Cluster
Create `m4` file on master:
```bash
nano ~/m4
```

Add these lines:
```
master n=1
slave1 n=1
slave2 n=1
slave3 n=1
```

Set proper permissions:
```bash
chmod 644 ~/m4
```

Start the LAM cluster:
```bash
lamboot -v ~/m4
```

Verify all nodes are connected:
```bash
lamnodes
```

---

## Step 9: Creating MPI Programs
Create a matrix addition program on the master node:

```bash
mkdir -p ~/Desktop/labs
cd ~/Desktop/labs
nano matrix_add.c
```

Example matrix addition code using scatter and broadcast:
```c
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define SIZE 8  // Matrix size (8x8)
#define ROOT 0  // Root process (master)

int main(int argc, char *argv[]) {
    int rank, size, i, j;
    int A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];
    int local_size, rows_per_proc;
    int *sendcounts, *displs;
    
    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Calculate rows per process
    rows_per_proc = SIZE / size;
    
    // Allocate memory for local portions
    int local_A[rows_per_proc][SIZE];
    int local_C[rows_per_proc][SIZE];
    
    // Arrays for scattering/gathering
    sendcounts = (int*)malloc(size * sizeof(int));
    displs = (int*)malloc(size * sizeof(int));
    
    // Calculate send counts and displacements
    for (i = 0; i < size; i++) {
        sendcounts[i] = rows_per_proc * SIZE;
        displs[i] = i * rows_per_proc * SIZE;
    }
    
    // Root process initializes matrices
    if (rank == ROOT) {
        printf("Running on %d processes\n", size);
        printf("Matrix Addition using MPI Scatter and Broadcast\n");
        
        // Initialize matrices A and B
        for (i = 0; i < SIZE; i++) {
            for (j = 0; j < SIZE; j++) {
                A[i][j] = i + j;  // Sample initialization
                B[i][j] = i * j;  // Sample initialization
            }
        }
        
        // Print first 4x4 section of matrices (for demonstration)
        printf("Matrix A (showing 4x4 sample):\n");
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                printf("%3d ", A[i][j]);
            }
            printf("\n");
        }
        
        printf("Matrix B (showing 4x4 sample):\n");
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                printf("%3d ", B[i][j]);
            }
            printf("\n");
        }
    }
    
    // Scatter rows of matrix A to all processes
    MPI_Scatterv(&A[0][0], sendcounts, displs, MPI_INT,
                &local_A[0][0], rows_per_proc * SIZE, MPI_INT,
                ROOT, MPI_COMM_WORLD);
    
    // Broadcast matrix B to all processes
    MPI_Bcast(&B[0][0], SIZE * SIZE, MPI_INT, ROOT, MPI_COMM_WORLD);
    
    // Compute local addition
    printf("Process %d performing calculation on its portion...\n", rank);
    for (i = 0; i < rows_per_proc; i++) {
        for (j = 0; j < SIZE; j++) {
            local_C[i][j] = local_A[i][j] + B[i][j];
        }
    }
    
    // Gather results back to root
    MPI_Gatherv(&local_C[0][0], rows_per_proc * SIZE, MPI_INT,
               &C[0][0], sendcounts, displs, MPI_INT,
               ROOT, MPI_COMM_WORLD);
    
    // Root process prints result
    if (rank == ROOT) {
        printf("Result Matrix C = A + B (showing 4x4 sample):\n");
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                printf("%3d ", C[i][j]);
            }
            printf("\n");
        }
    }
    
    // Clean up
    free(sendcounts);
    free(displs);
    MPI_Finalize();
    return 0;
}
```

Similarly, create a matrix multiplication program:
```bash
nano matrix_mult.c
```

---

## Step 10: Compile and Distribute MPI Programs
Compile on the master:
```bash
cd ~/Desktop/labs
mpicc matrix_add.c -o add
mpicc matrix_mult.c -o mult
```

Create directories on slave nodes (if they don't exist):
```bash
ssh slave1 "mkdir -p ~/Desktop/labs"
ssh slave2 "mkdir -p ~/Desktop/labs"
ssh slave3 "mkdir -p ~/Desktop/labs"
```

Copy executables to all slaves:
```bash
scp add slave1:~/Desktop/labs/
scp add slave2:~/Desktop/labs/
scp add slave3:~/Desktop/labs/

scp mult slave1:~/Desktop/labs/
scp mult slave2:~/Desktop/labs/
scp mult slave3:~/Desktop/labs/
```

---

## Step 11: Running MPI Programs
Run the programs from the master:
```bash
cd ~/Desktop/labs
mpirun -np 4 add
mpirun -np 4 mult
```

When finished, shut down the LAM environment:
```bash
lamhalt
```

---

## Step 12: Troubleshooting

### No Inter-VM Connectivity
If VMs can only ping themselves but not other VMs:
1. Verify network adapter settings in VirtualBox
2. Make sure all VMs are using the same internal network name
3. Check the netplan configuration (indentation matters!)
4. Verify IP addresses are properly assigned

### "No such file or directory" Error
If you get an error like `mpirun: cannot start add on n1: No such file or directory`:
1. Make sure you've copied the executable to the same path on all nodes
2. Verify the directory structure exists on all nodes
3. Check permissions on the file (executable flag must be set)

### SSH Issues
If SSH key authentication is not working:
1. Verify key was properly copied with `ssh-copy-id`
2. Check permissions on ~/.ssh directory and files:
   ```bash
   chmod 700 ~/.ssh
   chmod 600 ~/.ssh/authorized_keys
   ```

### LAM/MPI Issues
If LAM won't start or has connectivity problems:
1. Make sure SSH works between all nodes
2. Check that the m4 file has the correct node names
3. Try restarting the LAM environment:
   ```bash
   lamhalt
   lamwipe
   lamboot -v ~/m4
   ```

---

## Optional: Using Shared Folder (NFS)
For a more efficient setup, consider using NFS to create a shared directory across all nodes, eliminating the need to manually copy files (see nfs_shared_folder_setup.md).

---

## Conclusion
This setup creates a working MPI cluster on a single host machine using VirtualBox. It demonstrates key concepts in distributed computing including node configuration, network setup, authentication, and running parallel MPI programs across multiple virtual machines.

The addition and multiplication programs demonstrate important MPI operations like scatter, broadcast, and gather - fundamental patterns used in distributed computing applications.

You can now write your own MPI C programs and run them in parallel across your 4-node virtual cluster!

 

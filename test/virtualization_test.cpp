// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "internal/virtualization.h"

#include <gtest/gtest.h>
#include <hwinfo/virtualization.h>

#include <format>

#include "internal/procfs.h"

using namespace hwinfo;
using namespace hwinfo::internal;

TEST(Virtualization, HypervisorFromVendorId) {
  EXPECT_EQ(hypervisor_from_vendor_id("KVMKVMKVM"), Hypervisor::kvm);
  EXPECT_EQ(hypervisor_from_vendor_id("Microsoft Hv"), Hypervisor::hyper_v);
  EXPECT_EQ(hypervisor_from_vendor_id("VMwareVMware"), Hypervisor::vmware);
  EXPECT_EQ(hypervisor_from_vendor_id("TCGTCGTCGTCG"), Hypervisor::qemu);
  EXPECT_EQ(hypervisor_from_vendor_id("prl hyperv"), Hypervisor::parallels);
  EXPECT_EQ(hypervisor_from_vendor_id("ACRNACRNACRN"), std::nullopt);
  EXPECT_EQ(hypervisor_from_vendor_id(""), std::nullopt);
}

TEST(Virtualization, HypervisorFromDmi) {
  EXPECT_EQ(hypervisor_from_dmi("QEMU", "Standard PC (Q35 + ICH9, 2009)"), Hypervisor::qemu);
  EXPECT_EQ(hypervisor_from_dmi("VMware, Inc.", "VMware20,1"), Hypervisor::vmware);
  EXPECT_EQ(hypervisor_from_dmi("innotek GmbH", "VirtualBox"), Hypervisor::virtualbox);
  EXPECT_EQ(hypervisor_from_dmi("Microsoft Corporation", "Virtual Machine"), Hypervisor::hyper_v);
  EXPECT_EQ(hypervisor_from_dmi("Amazon EC2", "m5.large"), Hypervisor::amazon);
  EXPECT_EQ(hypervisor_from_dmi("Google", "Google Compute Engine"), Hypervisor::google);
  EXPECT_EQ(hypervisor_from_dmi("Parallels International GmbH.", "Parallels ARM Virtual Machine"),
            Hypervisor::parallels);
  // bare metal
  EXPECT_EQ(hypervisor_from_dmi("Amazon EC2", "c5.metal"), std::nullopt);
  EXPECT_EQ(hypervisor_from_dmi("Microsoft Corporation", "Surface Laptop 5"), std::nullopt);
  EXPECT_EQ(hypervisor_from_dmi("Google", "Eve"), std::nullopt);
  EXPECT_EQ(hypervisor_from_dmi("LENOVO", "21CBCTO1WW"), std::nullopt);
}

TEST(Virtualization, ClassifyVm) {
  EXPECT_EQ(classify_vm(std::nullopt, std::nullopt), std::nullopt);
  // QEMU with KVM acceleration reports "QEMU" in SMBIOS
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "KVMKVMKVM"}, Hypervisor::qemu),
            (VirtualMachine{Hypervisor::kvm, "KVMKVMKVM"}));
  EXPECT_EQ(classify_vm(std::nullopt, Hypervisor::qemu), (VirtualMachine{Hypervisor::qemu, std::nullopt}));
  // SMBIOS knows the cloud
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "KVMKVMKVM"}, Hypervisor::amazon),
            (VirtualMachine{Hypervisor::amazon, "KVMKVMKVM"}));
  // KVM behind a Hyper-V compatible interface
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "Microsoft Hv", .secondary_id = "KVMKVMKVM"}, std::nullopt),
            (VirtualMachine{Hypervisor::kvm, "KVMKVMKVM"}));
  // Hyper-V root partition (Windows host with Hyper-V / VBS) is not a VM
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "Microsoft Hv", .root_partition = true}, std::nullopt),
            std::nullopt);
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "Microsoft Hv"}, std::nullopt),
            (VirtualMachine{Hypervisor::hyper_v, "Microsoft Hv"}));
  // unknown hypervisor
  EXPECT_EQ(classify_vm(CpuidHypervisor{.vendor_id = "ACRNACRNACRN"}, std::nullopt),
            (VirtualMachine{Hypervisor::unknown, "ACRNACRNACRN"}));
}

TEST(Virtualization, ContainerMarkers) {
  EXPECT_EQ(classify_container({}), std::nullopt);
  EXPECT_EQ(classify_container({.dockerenv = true}), (Container{ContainerRuntime::docker, false}));
  EXPECT_EQ(classify_container({.containerenv = ""}), (Container{ContainerRuntime::podman, false}));
  EXPECT_EQ(classify_container({.containerenv = "engine=\"podman-5.0.2\"\nname=\"web\"\n"}),
            (Container{ContainerRuntime::podman, false}));
  EXPECT_EQ(classify_container({.containerenv = "engine=\"cri-o\"\n", .kubernetes_env = true}),
            (Container{ContainerRuntime::cri_o, true}));
  EXPECT_EQ(classify_container({.systemd_container = "systemd-nspawn"}),
            (Container{ContainerRuntime::systemd_nspawn, false}));
  EXPECT_EQ(classify_container({.pid1_container = "lxc"}), (Container{ContainerRuntime::lxc, false}));
  EXPECT_EQ(classify_container({.systemd_container = "oci"}), (Container{ContainerRuntime::unknown, false}));
  EXPECT_EQ(classify_container({.kubernetes_env = true}), (Container{ContainerRuntime::unknown, true}));
}

TEST(Virtualization, ContainerFromCgroup) {
  EXPECT_EQ(classify_container({.cgroup = "0::/system.slice/docker-0123abcd.scope\n"}),
            (Container{ContainerRuntime::docker, false}));
  EXPECT_EQ(classify_container({.cgroup = "12:memory:/kubepods/burstable/pod1234/cri-containerd-abcd\n"
                                          "11:cpu,cpuacct:/kubepods/burstable/pod1234/cri-containerd-abcd\n"}),
            (Container{ContainerRuntime::containerd, true}));
  EXPECT_EQ(classify_container({.cgroup = "0::/kubepods.slice/kubepods-burstable.slice/crio-abcd.scope\n"}),
            (Container{ContainerRuntime::cri_o, true}));
  // host processes
  EXPECT_EQ(classify_container({.cgroup = "0::/user.slice/user-1000.slice/session-2.scope\n"}), std::nullopt);
  EXPECT_EQ(classify_container({.cgroup = "0::/system.slice/docker.service\n"}), std::nullopt);
  EXPECT_EQ(classify_container({.cgroup = "0::/user.slice/app-gnome-docker\\x2ddesktop-42.scope\n"}), std::nullopt);
  // cgroup namespace: nothing visible
  EXPECT_EQ(classify_container({.cgroup = "0::/\n"}), std::nullopt);
}

TEST(Virtualization, ContainerFromMountinfo) {
  constexpr auto docker =
      "1 0 0:31 / / rw,relatime - overlay overlay rw,lowerdir=/var/lib/docker/overlay2/l/ABC,upperdir=x\n"
      "2 1 0:32 / /proc rw - proc proc rw\n";
  EXPECT_EQ(classify_container({.mountinfo = docker}), (Container{ContainerRuntime::docker, false}));
  constexpr auto containerd =
      "1 0 0:31 / / rw - overlay overlay "
      "rw,lowerdir=/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/1/fs\n";
  EXPECT_EQ(classify_container({.mountinfo = containerd}), (Container{ContainerRuntime::containerd, false}));
  constexpr auto host = "25 1 259:2 / / rw,relatime shared:1 - ext4 /dev/nvme0n1p2 rw\n";
  EXPECT_EQ(classify_container({.mountinfo = host}), std::nullopt);
}

TEST(Virtualization, Wsl) {
  EXPECT_EQ(classify_container({.kernel_release = "6.6.87.2-microsoft-standard-WSL2"}),
            (Container{ContainerRuntime::wsl, false}));
  // Docker Desktop containers run on the WSL kernel
  EXPECT_EQ(classify_container({.dockerenv = true, .kernel_release = "6.6.87.2-microsoft-standard-WSL2"}),
            (Container{ContainerRuntime::docker, false}));
}

TEST(Virtualization, ToString) {
  EXPECT_EQ(to_string(Virtualization{}), "none detected");
  EXPECT_EQ(to_string(Virtualization{.vm = VirtualMachine{Hypervisor::kvm}}), "KVM VM");
  EXPECT_EQ(std::format("{}", Virtualization{.vm = VirtualMachine{Hypervisor::amazon},
                                             .container = Container{ContainerRuntime::containerd, true}}),
            "containerd container (Kubernetes) on Amazon EC2 VM");
}

TEST(Cgroup, ParseMembership) {
  const auto v2 = procfs::parse_cgroup("0::/user.slice/user-1000.slice\n");
  EXPECT_EQ(v2.unified, "/user.slice/user-1000.slice");
  EXPECT_TRUE(v2.controllers.empty());

  const auto v1 = procfs::parse_cgroup(
      "12:memory:/docker/abc\n"
      "4:cpu,cpuacct:/docker/abc\n"
      "1:name=systemd:/docker/abc\n"
      "0::/\n");
  EXPECT_EQ(v1.unified, "/");
  EXPECT_EQ(v1.controllers.at("memory"), "/docker/abc");
  EXPECT_EQ(v1.controllers.at("cpu"), "/docker/abc");
  EXPECT_EQ(v1.controllers.at("cpuacct"), "/docker/abc");
}

TEST(Cgroup, ParseLimits) {
  EXPECT_EQ(procfs::parse_cgroup_cpu_max("150000 100000\n"), 1.5);
  EXPECT_EQ(procfs::parse_cgroup_cpu_max("max 100000\n"), std::nullopt);
  EXPECT_EQ(procfs::parse_cgroup_cfs_quota("50000\n", "100000\n"), 0.5);
  EXPECT_EQ(procfs::parse_cgroup_cfs_quota("-1\n", "100000\n"), std::nullopt);
  EXPECT_EQ(procfs::parse_cgroup_memory_limit("1073741824\n"), Bytes{1073741824});
  EXPECT_EQ(procfs::parse_cgroup_memory_limit("max\n"), std::nullopt);
  EXPECT_EQ(procfs::parse_cgroup_memory_limit("9223372036854771712\n"), std::nullopt);  // v1: unlimited
}

TEST(Mountinfo, Parse) {
  const auto mounts = procfs::parse_mountinfo(
      "30 23 0:26 / /sys/fs/cgroup rw,nosuid shared:4 - cgroup2 cgroup2 rw,nsdelegate\n"
      "40 23 0:35 /docker/abc /sys/fs/cgroup/cpu,cpuacct ro - cgroup cgroup rw,cpu,cpuacct\n"
      "41 23 0:36 / /media/My\\040Disk rw master:1 shared:2 - vfat /dev/sdb1 rw\n"
      "invalid line\n");
  ASSERT_EQ(mounts.size(), 3u);
  EXPECT_EQ(mounts[0].fs_type, "cgroup2");
  EXPECT_EQ(mounts[0].mount_point, "/sys/fs/cgroup");
  EXPECT_EQ(mounts[1].root, "/docker/abc");
  EXPECT_EQ(mounts[1].super_options, "rw,cpu,cpuacct");
  EXPECT_EQ(mounts[2].mount_point, "/media/My Disk");
  EXPECT_EQ(mounts[2].fs_type, "vfat");
}

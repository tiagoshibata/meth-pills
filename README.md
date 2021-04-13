# meth-pills

Meth (maximum ETH) pills are free, open source software to tweak GDDR5X memory timings on NVIDIA GPUs.

### The project works, but it's new and will have breaking changes. You will need to tweak it for cards other than GTX 1080s

## Why?

GDDR5X and other new memory types have higher peak bandwidth for faster transfers of e.g. large textures, but generally have worse latency. ETH mining does mostly small random accesses, which are greatly impacted by the higher latency.

## What kind of improvement should I expect?

Meth pills improve hashrate and efficiency greatly, with a hashrate improvement of almost 50% and a hashrate per Watt improvement of over 100% in a GTX 1080 when compared to stock settings.

| Timings t0,t1 | Power | Hashrate/s | Efficiency     |
|---------------|-------|------------|----------------|
| 20,6 (stock)  | 130 W | 12.33 Mh   | 94.8 Kh/(s*W)  |
| 20,5          | 134 W | 23.19 Mh   | 137.1 Kh/(s*W) |
| 20,4          | 142 W | 25.57 Mh   | 180.1 Kh/(s*W) |
| 16,4          | 155 W | 30.53 Mh   | 197.0 Kh/(s*W) |

## Which cards are supported?

Only Pascal and newer cards are supported. Cards with memory older than GDDR5X might have no noticeable improvement.

Only GTX 1080s were tested. If you have other cards, please open an issue reporting your success and what timings work. When running in new cards, start with conservative timings (close to stock values) and gradually decrease them.

## How is this software better than proprietary tweaks?

* Free, open source software that you can audit and build yourself. Some of the other mining improvement projects either have fees, or have been discontinued and don't have a reliable source to download from
* Re-scans PCI devices every 5 seconds to re-inject timings if e.g. your miner is restarted, or if new cards are added
* Much more granular control of each timing to find the best and most reliable setup
* Miner independent; works on any miner and frees you from dev fees. You can now have top performance using the open source miners, too!
* Our goal is to be community built to have optimal timings for as many cards as possible, using community reporting of cards and timings that work well. Let's all make mining more efficient and improve on existing hardware!

## Why doesn't my hashrate change when I tweak one of the timings?

Your card might be bottlenecked by the other timing, lowering one without adjusting the other can lead to no improvement.

## What are the timings, exactly?

Unfortunately, this information is only available to people with insider information in NVIDIA. From open documentation available online, we know it's in the framebuffer partition addressing (FBPA) register range (https://github.com/NVIDIA/open-gpu-doc/blob/master/pascal/gp100-fbpa.txt), which is described as "a memory controller which sits between the level 2 cache (LTC) and the DRAM" (https://docs.nvidia.com/nsight-compute/ProfilingGuide/index.html).

The software was developed by watching PCI reads and writes and doing a lot of trial and error. We have no priviledged information on what each register means.

## How do I use it?

`meth-pills -h` shows help on command line options.

## Is it really free?

Yes. Though if you'd like to support us, you can donate to 0x68bb64565f4fac6aecf406e127dbe04ef3e567d9 , mine a bit at `stratum1+tcp://sp_otmcc.youralias@eth-us.sparkpool.com:3333`, or just leave a star on GitHub :)

![Wallet address](wallet.png)

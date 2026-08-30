```bash
➜  ~ sudo tcpdump -vAX -i wlp0s20f3 'port 5001' 
dropped privs to tcpdump
tcpdump: listening on wlp0s20f3, link-type EN10MB (Ethernet), snapshot length 262144 bytes
19:32:50.639327 IP (tos 0x0, ttl 64, id 63197, offset 0, flags [DF], proto UDP (17), length 71)
    mini-mekka.37567 > 130.208.246.98.commplex-link: UDP, length 43
	0x0000:  4500 0047 f6dd 4000 4011 c5a7 c0a8 4445  E..G..@.@.....DE
	0x0010:  82d0 f662 92bf 1389 0033 7e65 5265 6d65  ...b.....3~eReme
	0x0020:  6d62 6572 2074 6f20 6368 6563 6b20 666f  mber.to.check.fo
	0x0030:  7220 6e65 7477 6f72 6b20 636f 6e6e 6563  r.network.connec
	0x0040:  7469 7669 7479 0a                        tivity.



19:33:11.914235 IP (tos 0x0, ttl 64, id 59236, offset 0, flags [DF], proto TCP (6), length 60)
    mini-mekka.52038 > 130.208.246.98.commplex-link: Flags [S], cksum 0x7e4f (incorrect -> 0x755d), seq 1308933786, win 64240, options [mss 1460,sackOK,TS val 1000223364 ecr 0,nop,wscale 7], length 0
	0x0000:  4500 003c e764 4000 4006 d536 c0a8 4445  E..<.d@.@..6..DE
	0x0010:  82d0 f662 cb46 1389 4e04 be9a 0000 0000  ...b.F..N.......
	0x0020:  a002 faf0 7e4f 0000 0204 05b4 0402 080a  ....~O..........
	0x0030:  3b9e 3284 0000 0000 0103 0307            ;.2.........
19:33:11.919377 IP (tos 0x0, ttl 52, id 0, offset 0, flags [DF], proto TCP (6), length 60)
    130.208.246.98.commplex-link > mini-mekka.52038: Flags [S.], cksum 0x225e (correct), seq 2795920952, ack 1308933787, win 65160, options [mss 1460,sackOK,TS val 3167456683 ecr 1000223364,nop,wscale 7], length 0
	0x0000:  4500 003c 0000 4000 3406 c89b 82d0 f662  E..<..@.4......b
	0x0010:  c0a8 4445 1389 cb46 a6a6 5e38 4e04 be9b  ..DE...F..^8N...
	0x0020:  a012 fe88 225e 0000 0204 05b4 0402 080a  .....^..........
	0x0030:  bccb 8dab 3b9e 3284 0103 0307            ....;.2.....
19:33:11.919417 IP (tos 0x0, ttl 64, id 59237, offset 0, flags [DF], proto TCP (6), length 52)
    mini-mekka.52038 > 130.208.246.98.commplex-link: Flags [.], cksum 0x7e47 (incorrect -> 0x4db8), ack 1, win 502, options [nop,nop,TS val 1000223369 ecr 3167456683], length 0
	0x0000:  4500 0034 e765 4000 4006 d53d c0a8 4445  E..4.e@.@..=..DE
	0x0010:  82d0 f662 cb46 1389 4e04 be9b a6a6 5e39  ...b.F..N.....^9
	0x0020:  8010 01f6 7e47 0000 0101 080a 3b9e 3289  ....~G......;.2.
	0x0030:  bccb 8dab                                ....
19:33:29.164951 IP (tos 0x0, ttl 64, id 59238, offset 0, flags [DF], proto TCP (6), length 95)
    mini-mekka.52038 > 130.208.246.98.commplex-link: Flags [P.], cksum 0x7e72 (incorrect -> 0x7d79), seq 1:44, ack 1, win 502, options [nop,nop,TS val 1000240615 ecr 3167456683], length 43
	0x0000:  4500 005f e766 4000 4006 d511 c0a8 4445  E.._.f@.@.....DE
	0x0010:  82d0 f662 cb46 1389 4e04 be9b a6a6 5e39  ...b.F..N.....^9
	0x0020:  8018 01f6 7e72 0000 0101 080a 3b9e 75e7  ....~r......;.u.
	0x0030:  bccb 8dab 5265 6d65 6d62 6572 2074 6f20  ....Remember.to.
	0x0040:  6368 6563 6b20 666f 7220 6e65 7477 6f72  check.for.networ
	0x0050:  6b20 636f 6e6e 6563 7469 7669 7479 0a    k.connectivity.
19:33:29.169926 IP (tos 0x0, ttl 52, id 24962, offset 0, flags [DF], proto TCP (6), length 52)
    130.208.246.98.commplex-link > mini-mekka.52038: Flags [.], cksum 0xc6c4 (correct), ack 44, win 509, options [nop,nop,TS val 3167473934 ecr 1000240615], length 0
	0x0000:  4500 0034 6182 4000 3406 6721 82d0 f662  E..4a.@.4.g!...b
	0x0010:  c0a8 4445 1389 cb46 a6a6 5e39 4e04 bec6  ..DE...F..^9N...
	0x0020:  8010 01fd c6c4 0000 0101 080a bccb d10e  ................
	0x0030:  3b9e 75e7                                ;.u.
```
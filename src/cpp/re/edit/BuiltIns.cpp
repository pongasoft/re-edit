/*
 * Copyright (c) 2022 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "FilmStrip.h"
#include "Errors.h"

namespace re::edit::BuiltIns {

static const char Cable_Attachment_CV_01_1frames_compressed_data_base85[5250+1] =
  "7])#######Y1sQJ'/###%gJ1<FsAB+-####[oDt8uM#<-qG5s-a;ffLk-B%>>j+##hSnO;M.B['tB*1#NGlY#`=xfL+/K;-6%BgL8ocW-4]r63D*@[']'U@0/@q>$^mJe$4S'^#_ZmA#"
  "GG2m0mD/B0NoPncsS1O1shI+4HiTRNPjl(5P4kN%d_lC+8_M`+He>mSl-'U*0%$c*kY<w`Q8cmGrb9ZAnS'f)v>+66fc<k'ZsuN'0-th.Q]n#U0+HaSO1wk7WR:$j+H(O1Q;t%3K6H^_"
  "N-NT/'hH$3>.fG]?_dJfo[r$(h,*Xa$PeL2YqRB?(QKRXdfH1<)+,PY#kABiGEmUb%H[]V0@ftaZa4Ehk@mVO_6j&ijDf/Q$WgS8^Nw9Wgk8k2Bu_9M1dutk.^B$AZ(V%66A]HdP4b02"
  "lo29/65UfD;`aV.@N8oX,:p3KYEssV[qJ+42(V71tMlY-a'96&:b^u.xH<(?fJ'f)_9;R'7[HnXtrr.G?Dr&Z^N8?U(FhDI.HGX0(OB^JxQIqC:G9ZWijg-=P]0O;*=a-4[Ppb48b$12"
  "aD4&M@j:BU''+tBCXp;8s(%>VhLvWL_0C)IKR+,H;P)iF6wk(?;Y'&.uv1wKmL><B:pl,>9/@A@eLQL<+7,5Y$+GlYNHX<VITS&,,Zg4n_CUiZ]ALXVDX)^T5b@6:`r^/3*<_Jf.WmqW"
  "qGTn*D7G9Wmq4G>]B_wVC1L)SS*0oN9B9%/w+]ahVf'2e4G$j)7@BdqA8VB_'6SmP]E*9C;vm]@oG^ro6DT4d:_5POLBpI4lhCF*.,B=keu01[S7()SD`:bh(uW;WC'u6cV,#E^e^B']"
  "ks]?UpetcH_`#w]q0Ow`Y%%$`S+BRO:=7n:4$$w7R:']jqCcldaY,*^u=S%C%$&uBq'<@tN(]ch'Fa(h@&*rWwSY_K-<[]JNkp`IoejY<.VjIp$soA,.7fBUG+<l15+Z?VI$###N3]1<"
  "0Ga5&9g,14Pf4W@(n+UB`4UG,c`.v0G7KC-A>GW/g8Vp2MKuV`9wFsuJ?-Fot<pbrfiDU=V.6DpaxZ&p7+>mn?i+uM<>L]jP#hGSQcO]7nkISm;QG*h6BdUbAxNVWq5d1PY$3<960R#M"
  "16h)j-ARt8*oVfbvh?wo^09;%uJ`:8amMmLv@HVPQ=^I?QMGp4mZpV5ZoG>IaRi8Z6Aom]^lM.^@Du;XIonW=]3fq4jSOMq6jL7jH,hXptws5vt7A^sF(N@hkPg5vH.puT#rk/RTV:)v"
  "4T$r]4a'+X[(Tq[&./>C?r&`6U)Fb=tt-J$1Y[]5H@m`/EL74OgR>7t)XG2X9qHPQ,fY67>^*,?C6Z*_lAu01;Q$'CmW>*aGJVQ26XDT9@>,qRd1=,S,ZstpE?@^,'NgU%g>bb`6g8-P"
  "n^l+SO1X-(A@gSf%($^GP$uNJ,'FD[;2*'+:cT+^DlOLp[gt`.[E.xc=woo[w$*IAF']'KC;o6a+%kuu/17U,7FCXpWH`%Txxe;6.sQK#.LX,O1.3Wa9?mbV,c#R5J)e.&n>OQ;(YL=#"
  "1eCYCF>v?'[&N0I(@QC$UmWr6LvSH40r44:rQTt$pQvYSrs-0)?SHo&q>b'%wO%Fr@8;0?7f@0nxJ>9`cW@N^)sUj&+N>Xh(%$&<C$C@13x6DZfR:;Ed=1JtH8[e'<C3[%@GTfN`e#<f"
  "baqGRCV*wOqT/ceN>'L<(1u==qnUNQlZ[>4MtB9=:5'n?<Aw1U`0C:[VNJ,P6EwV2(27H*XV/NrQ6L:+wY1b?+`B?@YJRYr1-IshQ'QXbEP2I/&4]edG)KB(d&CL$4;Oj:*Fs(Gfc/`l"
  "SC>.Ns>WoZ3VU9c1j$TKLU5@aB=&?6+Sm,6vYVIB[?/9>Q:9viZMFi*$Mw01]DDZ*Ao.bB_;tHeV4eo/iatbmFIfotTdp-c3)#=q@k+M2oF3-?NxQ#eFD@^6LWw&?.OdKa6RDh6`s=1/"
  "?;p<^CPdF%@[MEW1;w8pnMppgwu1R/h]'p&Bn-V3e=d5XY)m#N.SrH7snCrQijDr#+mN5,=T5`ehwE>IRGJ6S-:hS_aAM81+gR)cS`&6k.Ys4k4]uA/eRkbJlFIAD.]Oq/aLJU&V+pLU"
  "P<WpCGo^#4%.+N]9dUfFNKuJ`)i4S;[6Kw?:4%FBRnC(0&[Lj]2SburZl?&`J0Ananu5whr(O7%SNhqe082J*WoPKu@Sr>mU]g8qG>b?[r%)XjMRcws)'6a_fZ^aa`b@Ih;x>H]agd^+"
  ":hdK4LcH3^)QAsOdevkIs<GBT%-#6p-HBq6KQC<4DrqMjGJZGhH7%ReVu%_(lUB;6Ua'M0cI%utRmXMXE@_WFj1M4,P'tO?MF5;0)KF/n>bsNWCFUHM>H4*qbsGdsaElUXI)>q@:W'tt"
  "@EFR^1.XiCfo`i0cn#pK##kM+uh*wA7kvar(LA_qL3I6H#',_,Cx7E_Sj<3<9GjN#2*[n`?8;VEBT1e3RH8_bwZg>58EP$qLt;R05e-hiB=SEVGNL^g?dRZ>SPKw_Jt#IDKai>OE`VnL"
  "J(:,>/LPBJO1<kgC2GsH5(I/U7gU5;]6f_@?T<hl&:lvN-9a)JN=688$@ToM.tv1fFf'I`G'n(KDGw2mv6h'o>'I-7TV+WZP@xgiP;CBuAkiN?c;5md3=W_pOBWArh-5$Q$8gv6)Jf]/"
  "Zk&TUS/?iaO9=5?ST5/]Vr)<KVU^-m6Iot6QRm/b7,:**v@_(Wqid2H+L+;c.CTnZB.C>l'=vms(,&Li:7<PP79B?W/pVapQi.62'v5^FHA>LO9oF+3E1%]2G;/U=PUU3s$9ks.)Gae]"
  "uNjI^LUNoXBR@s62YKBFZ<:vt;DT#n*#OS7e[@;L&=5vIvWG64DsgcTegQwCF8<_fBEXucp]QPDABR8tHMP=*]*OgU`3MvplR6=1ASEK>H*jV5TsK-jjH>AM`@uQ*.s*gS40Kj$oXPWe"
  "du%#Z[,7(WScB/#B@@adYKnKJV4A3+Ml]U?\?.@FEH/mo07<QOj:om@hFYUra#_29>2rToa$k<IqtJ^$-SOoY$N'LvDqltr;95l;=B7xWOh-;n8iG7s40>$oDBHTN,dfE8^Ch5'rOIQOj"
  "^qgisIn*`YjY>lu(SLl.$p8@J4(snkPU%;Ht,kP(^D#Z&@^Z:pqv[F5x78%JrGVC;8(^khPYqc#0FraaLrG;tW`hhXnuk'#glm.=nU_d1gPRqp<h@7[-U;2t]?PP'@/4gS``T_s?2@ed"
  ",g$/uj&I6&<+A$kD6[3WR2g*nC2GQ<7sut3n&//#YmbI3v2L_j#5ZgLu.p6(LYa28oZ=^-;:u0oSTc-JV].tW[<I%QQ'H9m^C%`UURH?jMb$PWp#i;Nl`=e2_so'9,c)JresN+EivmLi"
  "u`atpr=k3da$ww&kJ'lktv'Mnc^#Xk'.*_J0QmLmju&bu^H`rND0Cs+2l9?Kl7ILLpDQ>6s+O'Klulro*rt_U2;andAoNu7`Od+-^?KRlHA65L,o@AH27(Ie00C6=9wYaVQ4OW0W9F&G"
  "FEh[qe1kf=Ykh:#VbUjN@6%B/.(*7[E1O7gb-gqD_As.;l2H-gEb3Y&e:n8NL#RDEAaat*m0>uTtF#:6Q/Db9f[X'SQdTtD;^jRQL2-M=6x6=L;k_UG8uI1F$GQeN>G+F[$3JgJ69Kg1"
  "n[qx.rYvcaG6$0WK,[MlCgt&,5wcg[rMr$VRa/MjVq$xET'2<nqa?a'PQv*oLe>7+iS5]WhPbS<r;+6TcbH/vKdODIod8'/maNkG9?Yw)XAnSP7iE_.YBQwEmK[&v3'05MQ*fpP5TOd6"
  "i5U(Qax`#0x).I7tI?`:@XTD%uLnq]'91S=nAx:D0->'Y8#ceCvoN&GrDoej-W97MTi:i@P&He2X.mFt`2DK:Z]APkb6NK_%sH_iU61N1]7Z_K1rRWcqVg9)S/q[]G]pGKk25f@@*W;k"
  "1]`X68LAk6r.;T0%MGOnW%:vk$URtPE/GWIMwNOqD>/Adw(%6b'ruls#vQphfY/Z*_A%'[)P>`Mq0NECg_+O7Y[:?LUc?I&%HeEDFm+VRd&0%5N6O;NroT`4%fdl&lGZjPnl4RY4uEWG"
  "AoBBT4<?PL;KL.?1@JLr5jtCM%Z['9gmim912oZWJ^&J;cdi5R7OB[rhP>1b/Sf,r2feSJ3em5>:BiAY'?PPncFSHnh>JH^@A,M$=Kf9GNJa3^uDZH3<e,$`tLQBWi0.$2Q`?6-QBV,8"
  "qkMt$_^qTG>XkgGi((BtH,xtkpNP$t1fRBlDrPS1-lmE]sXrb=X&;GRi&D$_V7RJ=]@PuA3S.Q6A3$<p])m7&`h'SBZ8ZisXhCCi6Jxdq'0j$uEVIrYmv+teKh4vb$fxI;Iqh/Bk#_o^"
  "kQ3LMvURXno%s7N@@4-=V%WGB9nIaU62_X^$Z`O?f46G6m#MO>bxKVI,agb>M*a]UAEDqDxE=70Eki^YsiUb$hN(.<(rgd=?04E?iADf>BS'vDxxNaN.5epaw.8bAmd101/u5NPtVm2T"
  "Hsx>j4Q)qgGKsT7VLkT@sx;c[g#Wo=1W/&*+QICbr$[v3s*Y%f#V0fkSdWm%wNi7&oZV]?>/NDoh1HuUN1EDQvTd&@]`:rG>ad&@n=1E80[V:sK=L%EmKSnf_q=,k];)U8*L&T5eBl1S"
  ";$u8+Fu#%skOu>s92p8#3-$k`1_r:3a5Lg$`&#gGk4_eG*r:,crHVa1@Y@ek@d%]Zw]=u0x<d<7O5:&$>?VU9up664K:9'Sfd*GdGXj=_Jv56Iu)S+8V4MlIT=MiqAo/U)kXdE_p=8aH"
  "fpf:I(=8Q'(9uD*7[.2rKiUl:B*uuWUhwJkjYPE/SBO$,^6C5?`;T1I5cx_;pQt(S/YN]WpJg9jnfSg_cox(Go2j9i(exn2kBsQs,_FLX;H4%-L<g$'_Y9pkF.gO5C#%JB>hfF'WaCNR"
  "W1;8q[fSGI@jed:Nk0x3OZ7WjNNebLFLv0;5WF@U`H@:Qes)=1@Pa]>p:LoM9^>v>3<iOcZ6c+0MP@=JZ2pi__iiK[D3?t^>CvBjnG)-FSTFE:Z&C^pW@&mU:[].g/?ZRF%gu,o6]Lwh"
  "w[';3?jp/`Z]2XN<p%b/uO_7^D7[4`-[Yf,L42],12l2,%Bpji,r+S)M<Vodg+9iKl52JO>1OuGvd7DYfSXensV%sFq<:?_.^K`]LTpE-Y_1ArU;Zr@)LMs(T^=hl'r@lN0o:Z0h_G+f"
  "R)V6VaKc@%CAOE8cjD%FBj@/T)V`>fpR1Gj.I84.8OU;#mb:J:T%f#[M.Ne$n=CxnP####";

static const char TrimKnob_compressed_data_base85[6165+1] =
  "7])#######iYv*3'/###%gJ1<FsAB+-####[oDt8uM#<-/BpV-ajJe$7KYPC66YY#F/s*>q4Dt-Ng[%M'1ZwdL`XldGJ4Pd<&S4dA2]4dGVOldGD`(1Q$^v7I-@L2ge$P;At7c*.1ZUb"
  "<,liCW4dZ%[oDMenw.$UX.uJHku7&@75[J[X(]]LovQ&J(q*[`sQm8OCR*7Ns1KZAt-+@`BF^4YA:d&Ta'mxOiqeta<=l1ZgYF1$;_jp%ZpN+.@9%rLo(Ek'>l_;$rtXgLh<IiPvr:eM"
  "v9hF*Q^Ph1qrRL(`F_1P,lu&_bG4sL@_Ss$at0XVE1&m+lQkp%B^Ph1@<.%MHM5$`D<nf[HS@xL0&Jw`d:i^_Bk,9X(5BT%[H1tLYkFT%g1FI)'=ugLhPo-)F;qiebwd2UXi^ZKedVqL"
  "+`Xv-)H1nLLFWT/`Iw8%gpWoRccc-6,6V9CDUnO2]ehF*DfU;$niOjL,w(x.W)c#-HqnW_cTXAMI2eTM0/qcRb*GlYqeuNG-C..)>PuY#9T&LO4ss#KXG*c5j*tu7H+=9/8jaI3VBtu2"
  ":0@L2;J04EgYO&Mq@j0(m#7UM+0aN4C>X6NH,Y:vk`FjLtuX00-ahY$npsY-<N?*MrLp;.L3s%,VU*9%Z;@-)A/?W.5igF*UpLK(;;29/CO)D+bD+jLc:?g'Q696&6^O>-[Kwg)J(fQ0"
  "?(6c*&JO.)mFv]6T1?c*xwc_+6CZ(+:ol(5qqON1)4>n00t$12L-378_seh2W9]],:I-U7dYT+4E]B+*qk7a5R#XI30@c31:Z2t8xCBh(>U7&6=8=3;[n2i<M;]6:2b@T:%>Hc4B@hk;"
  "/chf?Ur>XaQmsG]I#-3tR46Pd0t#lEUKTi3x/<'sZ`awjA%saT(^p6D:Eu(?ZUWOosQ,[`5+GlYoJvPOB=@UM%icWBmU],hnkbRX`VHUWj#[aIOOo+HED.Sm>J'ieU`VtVNKVS%G,^79"
  "r$###s3]1<AD2JQrc%jPjSiMPYV3**omahuPa$3>%,nCF&;VOL1]fD,2,@k<HoLUDn-p[0oFWd]8:5'6gKIYkT7@DJ.F:(@tjSAVjJr.Q7/K$KL$t.=d5C6nTCLNPkMk&Ah:Z0h.g&MG"
  "[wE$u>JpSc;>;&UT.MtL;2h4E%Y,v8Ehf[u3Ztlnp'/$UH+O'/l;1]&SS6tHFrc)>^2*6@xTk@I_%1piw^^u8Uk#x[7Q>sY;g3[k$Ya$MhjF01x2bTq^qim(m$jqil`W:eUYBA,32+,v"
  "OGeX*qP;X42Td7p)@4VU:4RxpM2jWs0p+=oTE_4hS,P/e8Ie(0h(jLg[)79SBW=LW3g7QXf%F-LQvdw.92`omHHUx>A>bwk1TX*B<0Mg;[5/nG;@vniN+fNat=.JGRwFt?3H'RTd8u:k"
  "k_/_]A(`2=*xaESGwlm:h'HN>8M$hW^S`B=q]<P1d_%:MCY+?g:?%E(+aV(I7qZ;=(TGAA^g5cCSAUqT].L1<RvitiQfl4>gGeo$Mt'*=NTv3Ka&6QXuqkWdb-`-WT-$GLcu5Jl#%@Zs"
  ".xB&[8l5/8.D<qR(?<vtpx=u;4?`K-:51m869%3$4YXGe#nN0,Hk]#MS`M0k-Up]gsb+<fQ$'UA%aIog%D`qF2c#UqP>N0L-JUp[4ufbrW]PB^rMu1ih-;2bI?<Cka9^%jY1i>FUrtP0"
  "m@rKrWU+(XI%[@OH/Y>RmjP=/sHYw1Ln3[4`c4<s[d/q8PP1rph[:RC1)`01T0cgS]dBm]LJ9YZ05Q15X-dN@nan0Akvno,%ZCn*.W-gr*SuqQc7l+`+,x<2?#84twnN&,h#8vos7S%^"
  "3A,jfGQx_qS90H9)W*>%J*Th3cpOuTS1*QgBnuHh?B`ZU]D1?YtvfN4-jCrM`oghiLkONP?&,HMaWwbaNnSLuduQ(WKXcQVtWco_-CvYpW*uUqO&KdrO:k,0etXcbi8_v)DcO4%*=^LF"
  "4M?EK9mUBi:;a+D_S^MapnmS*6tT)vt:07fh$[<fJOS+bW^V(]s,-Drcf.<[gU:2nLA/RR8k^t_;X#3=>&H8Ik51n0AEg61A;nC1[OlWdh%u,mQ(40IN9>oc55/0BQa]0VV-3e8=37uj"
  "&7D:Z5'j%k?QR?h5K@YQIb^]NK4.qC5AV-W^D_q=0,uUDm9&hIn8i.Z61Kr'4[j.HUAIY7R6Z>?86Q43Drv?`_v.pFcp7EJf/&aD)Zo?XhQ$)U:U`jIw)5l.O7_c&x%kJqdHx)bH=#+t"
  "(P_B']ax8o*2hjcC?wW&:K3FHLw$kWeqPd;=ofA@_fH$'O[ur%uJECSUk<Bs+6[Lci[mbQ9++#Qgwq#]-(>D(jk<R64&uQB<j;*82i;b14Dd^8iB]PL.tDYQbRm_cLXrm'pB)'+J#J'Z"
  "[daCD*1*x+8aLktg8Pd,CRo?ljQB_R7mQei-IfksLr2qVY%BnXV[uH)Ex9)IA7u2N.7s`C*j3kU'YiA-D7`$T:o)9G_^cJ(pw:qb%A(cLQs=T7/[O4adA_Vc]]ot=YNN?'M6POq)8Do="
  "N&=].fYC-uUHoU]4aGC/5Y#54QeTd,%D-P'/;f9hg;vIN;Ncx;)n7g^4*2+`Fp=Apx)u=YR6#HfKpXYJ3xa+)@wp#L$l/JMkvqK[+MsI-:LAL#p_J*m<%-YTAI>>MI.jV6WBiuhAOegQ"
  "X`_#'Ul27]B7w4D/,-dgce)6hL%b5N@bv.V5.Oeo1nc>E0hk`=@[KUYDU(-Mc@G/Ivd3^ZBXH$WiG46N(HGlgH@]5WM4q%t=Z;X`+kAi$_0h<bB9OZ<B<pb7`WGapSnHF+Ge[KE8$aQC"
  "N-lwANR]Q0fZ9b&:e=LMSV=%@QU]YAlXVL''VvA=PI8I-Rxu]CJTkH,hY-:e)uh'O0hhgOt$u4ko8eX[O6+vod)+UkX^4sB-4nXjaVGn2*%VN>mFE3D>a2iLmfL@BCt-/=_SjQ'3R+`6"
  "IAtm:1vxD%W.l3'rLv_dZK;=kLO`mbZ:r;-r3@ZO5TI3Bn2StVjTD;G$SPL87&_ok+:Q>`6:,_CJ3o>I&0XNRi),^/r`##Aqr[(Zk^PdZFb=*mbKC5)+6s-eJYmPf^hL?3Bs2uie7P%F"
  "pY_5]Ni=:OPS;+&HZmP8.m@f;qo1`#A?[H9sPGR4-5SZPd<mZ[[s2+.lO/]KWUDk7:Wj0(E8r%:2Oir]u/@rpjn)0F)ak$61Q`?IUc;0l9E@T-eSc5C^$uw`aA]4^N/CioZ+<B109RCA"
  "en[ME<555?&kHmjGB@1a3#o=ccpoQ]?@#'L6sW;btc(^6Xi,LH(c[0.Y.5FPdPR)GXA@MkJn`niwYqHMC18.86LOA8V&pbN'1e)_+dYtPj%5.RD/YFI*@r(?o+U?Eajwh+)%iTpk$qpG"
  "Nu^piu?[iL%j)TQnnWk5ZhE$S83Xdk3Cn-B[VD#RBHBmLcA7RNP6+*UNBhfd^X@.5c'WE@u-]=gBl%cSjiuknVsPXH?_C5:B1]`nSWY,$1,[J8&A<-.Jq+4fX&;&_oq5)N#M6>E77b<j"
  "R69h@9'a8bZ^9FiTAq&1,'1>D1@UqEdk3Q['QkR=2(qYQR,twir`QZcfB*MHOeUk5Jab:KpGjT<'Y-*s6_(AEQZb4gH>hn4A$YH#M.9CrN$xOAev#'=23G6fnj/qVx$$bGR5qT(6c[]b"
  "hZUn%CW?/P3@Rk1Ij@v`S=7QsE^)=kD34^DeW#ZOArx'/cwX2nd;suV(*eX3_^C)^ZOpP7WQquA`huCqWn0oHeqJc`Gt3UNpwq00^'PO3gX]EWfbHQuu4sYdrf94maIJ7YK8D$Bp[+hH"
  "q0f1B,Z@XIP+nCu&>k6G4G$Ghnj.U0[#`Msp;twT09?lfQ6Kc+7@.$8vtX<b7?k]o555u[qbLDY@[P0&[1JA-jtJ]Y5RK:JQQsp?mKn1&ThV975'Ka0DCMiX<40uiaZn4bx3>_+)3x(W"
  "I:mj>`ZZs?2Z%5(vK_=3.NsKUMqumY;dda3SCWMY2$BQa=w$<9<n)g<&9x>Yf9b,e)o3.PTsGCmdVkd;e(;T]@+l$t^#[N`,fJ4Y>>RbGfrE1XIC[^HX'p8M>t^+mV`w&v3B#l4W]WYd"
  "Q>VZ*a`xLFe:ThK[eRP^qdXP1'vhv7Kfi%ltXDk7CGkkKQxq,%t$'E_SL`bE91Y)lu:KwbX-NR7iiV1:9r)NW[SqMj)6dXnh4)MkcG;hFF'6<$'$+?7$?dSW2V04f_wO;@2r[I#U3HS7"
  "7^h>6Gq%8nnJ]3_>I9(-ME80K`G_8b]$TEqU2/9DTaW-7]`GB=YTvj_ljMM-+bPihc)U:I]VsD3Fa$PXE2Dl-c2AT+7Jv=j-L2?tD7_&BbI#i>2emn);O6iK$b-ZOY3'-@?I9<dq[0V&"
  "rGNThWsp6dV)_-05>FD=x]GdNv+q6-v5;:PU2C(,f%hENS-D$g#pfGA$gFLtDs<N]&dB9Al2_r79adH-Y8o9kv>S+B65:I/5MLd17jFJQ$rB=Tq=-'twgRuODnaJ&NjMggT<c085KHV4"
  "['D,8/2Ojl6thnQcc.F8dt24/`5Uctad=k*suI0O7DJ+$ZK<kH;Kbe^2shmP/9r6f3+k%/ic5mVGYGhKh'`+IP+wXjPk?1$b]?\?$I`;cs)8h2[_uOt?)?Clm6ok(WxS(<@jc$5K&n,Rj"
  "32mCOo$^C]`;qSL/9u3153I-[DI]gT3P'shno?/gd++i%U:*eWWcbGq1djHg#:qg=`_#?8;=RdM/faQGhi8lQKN3K)v$iqK7;$cG5Oro2=7n&Ew@W&48CJGICWV14,S^>)Ub8d3n[fe2"
  "[fg;6'*Eb&-vNfPEmv#k;=+gY1iUGe(xqF5A-GWgm-^?B1XA:4+a.j?.k#Y65cPCAI:W3bt`3/K6KqWN=^Vo+)se#X>(.hKn5iqP6/Hh<obw+(tgc$R@C+_eJEj/%5j7lDx6d2<=DB<q"
  "C;3ZPdnQ/PCowf8x0Z%]&=o$kx&c+ZKIs(@p6qB&KAm((kK<+#=Cs$N:6(mfg'M7FQEaYVf`4x/lil.MfxetGL^9ibV8=?:<ZI=I2cw1*.N<h>UBG`q+K;p$kbc5Bvj)3d<h&W+q#Nvj"
  "fZrqL?i7#X]C/vVXQ8]9Krs+#<Mt;)H$f]grdD7QKBS=-?*t/]5Rp.FV>LaOOvU+TmRj0+$#Y+A%mP&NkVp*fZGh(2Zu-#p;g%o$Wo=)11T)AXG%-<_B=1^gTmf*Bo$`2ZN1B7DbOc<m"
  "J^vXB)QS`,-ON^XmX$)-J;fxBFBV#k$8Xi<squa*u=d6fD:<;%?xqmMTaHt^X7&cX)USciSo:i%cuBUWOm5UMBO_:Gf0HxZ^4bobjuD4>k/+2pIF1$6Ouj,`A3R5kDJDx#?_@MfhFZS%"
  "W>5XfG2Mj<SA(]H<'q]LP3OFW0e>X;W5B`PKRteq92skg-EDq@^wT<Lbal0iOx<954/>-qTkvhD>GYpYa/h8eE=W#_f7/m.(E:oQT?T1Usv_7HpnOO]1e%wM.A[-E<DHK&II5O$p>5qV"
  "egeq//HD[?H3t:[AhuOIceFvB&jb0(_-jhPT5FpMrw,OgC&*%p'4T=DdU>r=aERO^5-hJoC[,`XbZ)6LQ`R0:?pN9,fv+[t2?C9V/@_qdwbt]mt0/2O,WcG6xLdE8WpGAPb87,345q/["
  "s;%#?,KFH4`k/G<MJ5f8-W>QtSa&50M.ZM;Cb(q'@XtjF6DH644;Pf`0ko?[i2/<N.UUo16AaORGWhr9Niv>lO/(JfwcqF4bp?*<65iO*o6G.<c#[H'lIM.+K*3/`JH/J4BL>kfU(c$6"
  "PhZa(+=uGZ^nVc16w]S+rr/6?GTYibxjIY6Ct7XC;g+slxaiAoa6YLttRru`x1aISI-*SiFV1>AYCqcS7x/AGkOW@Uj*0T<gBkOWb+QMY;B8Zgn.)MrFkt[/AKQY`#u%M>'QGv=?%@%9"
  "U'fLmNt.hJTS/KkoU.jB_pVs$C7hCi$EfR=/0(8Hv7VS5/sZFYTZmm=IG]^gx0g&rXZZx[f%d0*w5*fL0f:=-N+5Wd7?rw@JJ2=Q7ja0UtaYce(9kQ)$4k;9E1MkrwQ&m/hbixJ(sq#>"
  "oktGl9I<8]:N;f[5Ho8Y?vV(4Ob;%eJ?M;Rs;d-0bxI.e,x)a)/lV8MeNplV/6F5?HMw,O-o,k:d^A9GCXH:q8<MIakQI0SMG4PZeWI1vB5&:7._9+&kNL3P#$###Ub90<d$bsAh=$`*"
  "K7M,#";

//------------------------------------------------------------------------
// re::edit::BuiltIns
//------------------------------------------------------------------------
char const *getCompressedDataBase85(FilmStrip::key_t const &iKey)
{
  if(iKey == BuiltIn::kCVSocket)
    return Cable_Attachment_CV_01_1frames_compressed_data_base85;

  if(iKey == BuiltIn::kTrimKnob)
    return TrimKnob_compressed_data_base85;

  RE_EDIT_FAIL("no such built in %s", iKey);
}

}


// File: '/Volumes/Vault/Applications/Reason Studios/RE2D_Stock_Graphics_1_1/BuiltIn/optimized/TrimKnob.png' (4914 bytes)
// Exported using binary_to_compressed_c.cpp


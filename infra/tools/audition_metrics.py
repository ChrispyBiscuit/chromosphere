"""Measures renders from infra_audition (interleaved stereo float32, 48 kHz):
    python3 audition_metrics.py dir
Columns: loud (dBFS, 90th percentile of 50 ms RMS), crest (peak to RMS, dB), the share of energy in
sub 20-60 Hz, low 60-150, lowmid 150-500, mid 500-2k, pres 2-6k, air 6-20k (percent), cent (spectral centroid, Hz),
lowside (side/mid energy below 150 Hz, percent: low-end stereo), width (side/mid energy overall, percent).
"""
import numpy as np, glob, os, sys
fs=48000
def band(P,f,lo,hi): return P[(f>=lo)&(f<hi)].sum()
def analyse(path):
    x=np.fromfile(path,dtype=np.float32).reshape(-1,2)
    L,R=x[:,0],x[:,1]; M=(L+R)/2; S=(L-R)/2
    n=len(M); seg=int(fs*0.05)
    rms=[np.sqrt(np.mean(M[i:i+seg]**2)) for i in range(0,n-seg,seg)]
    rms=np.array(rms); loud=20*np.log10(np.percentile(rms,90)+1e-9)
    crest=20*np.log10(np.max(np.abs(M))/(np.sqrt(np.mean(M**2))+1e-12)+1e-12)
    W=np.hanning(n); Pm=np.abs(np.fft.rfft(M*W))**2; Ps=np.abs(np.fft.rfft(S*W))**2; f=np.fft.rfftfreq(n,1/fs)
    tot=Pm.sum()+1e-20
    sub=band(Pm,f,20,60)/tot; low=band(Pm,f,60,150)/tot; lowmid=band(Pm,f,150,500)/tot; mid=band(Pm,f,500,2000)/tot
    pres=band(Pm,f,2000,6000)/tot; air=band(Pm,f,6000,20000)/tot
    cent=(Pm*f).sum()/tot
    lowside=band(Ps,f,20,150)/(band(Pm,f,20,150)+1e-20)
    width=Ps.sum()/(tot)
    return dict(loud=loud,crest=crest,sub=sub,low=low,lowmid=lowmid,mid=mid,pres=pres,air=air,cent=cent,lowside=lowside,width=width)
role=sys.argv[1].rstrip('/')
rows=[]
for p in sorted(glob.glob(role+'/*.f32')):
    rows.append((os.path.basename(p)[:-4],analyse(p)))
keys=['loud','crest','sub','low','lowmid','mid','pres','air','cent','lowside','width']
print(f"{'preset':28s} "+" ".join(f"{k:>7s}" for k in keys))
for n,m in rows:
    print(f"{n[:28]:28s} "+" ".join(f"{m[k]*100:7.1f}" if k in ('sub','low','lowmid','mid','pres','air','lowside','width') else (f"{m[k]:7.0f}" if k=='cent' else f"{m[k]:7.1f}") for k in keys))
import statistics as st
print(f"{'MEDIAN':28s} "+" ".join(f"{st.median([m[k] for _,m in rows])*(100 if k in ('sub','low','lowmid','mid','pres','air','lowside','width') else 1):7.1f}" for k in keys))

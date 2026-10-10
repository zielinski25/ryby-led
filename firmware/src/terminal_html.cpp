#include <Arduino.h>
// extern required: C++ const at namespace scope has internal linkage by default
extern const uint8_t TERMINAL_HTML[] PROGMEM = R"RAWHTML(<!DOCTYPE html>
<html lang='pl'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Akwarium LED &#8211; Panel</title>
<link href='https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=Outfit:wght@300;400;500;600;700&family=Syne:wght@400;600;700;800&display=swap' rel='stylesheet'>
<style>
  :root {
    --bg:#060d18; --surface:#0c1829; --surface2:#111f35;
    --border:rgba(0,180,220,.15); --border-h:rgba(0,220,255,.35);
    --cyan:#00d4f5; --blue:#0070ff; --teal:#00b4aa;
    --warm:#ff9f43; --red:#ff4d6d; --white-led:#e8f4ff;
    --fs:#7ad4ff; --navy:#001b3a; --text:#c8e0f5; --text-dim:#5a7a99;
    --glow-c:0 0 20px rgba(0,212,245,.25);
    --glow-b:0 0 20px rgba(0,112,255,.3);
  }
  *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Outfit',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;overflow-x:hidden}
  body::before{content:'';position:fixed;inset:0;background:radial-gradient(ellipse 60% 50% at 20% 0%,rgba(0,80,180,.12) 0%,transparent 70%),radial-gradient(ellipse 50% 40% at 80% 100%,rgba(0,160,200,.08) 0%,transparent 70%);pointer-events:none;z-index:0}
  .navbar{position:sticky;top:0;z-index:100;background:rgba(6,13,24,.92);backdrop-filter:blur(20px);border-bottom:1px solid var(--border);display:flex;align-items:center;gap:0;padding:0 20px;height:56px}
  .brand{font-family:'Space Mono',monospace;font-size:.8rem;color:var(--cyan);letter-spacing:.05em;margin-right:28px;white-space:nowrap;display:flex;align-items:center;gap:8px}
  .brand-dot{width:8px;height:8px;border-radius:50%;background:var(--cyan);box-shadow:0 0 8px var(--cyan);animation:pulse 2s ease-in-out infinite}
  .sys-header{background:rgba(8,16,30,.7);border-bottom:1px solid var(--border);padding:10px 20px;display:flex;align-items:center;gap:12px;font-family:'Space Mono',monospace}
  .sys-header-icon{font-size:1.5rem;line-height:1}
  .sys-header-title{font-size:.9rem;font-weight:700;color:var(--text);letter-spacing:.03em}
  .sys-header-sub{font-size:.72rem;color:var(--text-dim);margin-top:2px}
  .sys-header-sub span{color:var(--cyan)}
  .sys-header-badges{display:flex;gap:6px;margin-left:auto;align-items:center;flex-shrink:0}
  .sys-badge{display:flex;align-items:center;gap:5px;padding:3px 10px;border-radius:12px;font-size:.68rem;font-weight:600;letter-spacing:.04em;border:1px solid}
  .sys-badge.ok{background:rgba(74,222,128,.1);border-color:rgba(74,222,128,.3);color:#4ade80}
  .sys-badge.ok .sbdot{background:#4ade80;box-shadow:0 0 5px #4ade80}
  .sys-badge.blue{background:rgba(0,212,245,.1);border-color:rgba(0,212,245,.3);color:var(--cyan)}
  .sys-badge.warn{background:rgba(255,180,0,.1);border-color:rgba(255,180,0,.3);color:#fbbf24}
  .sys-badge.warn .sbdot{background:#fbbf24}
  .sbdot{width:6px;height:6px;border-radius:50%;flex-shrink:0}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
  .tabs{display:flex;align-items:stretch;gap:0;height:100%;flex:1;overflow-x:auto;scrollbar-width:none}
  .tabs::-webkit-scrollbar{display:none}
  .tab{display:flex;align-items:center;gap:7px;padding:0 18px;font-size:.82rem;font-weight:500;color:var(--text-dim);cursor:pointer;border-bottom:2px solid transparent;white-space:nowrap;transition:all .2s;letter-spacing:.02em}
  .tab:hover{color:var(--text);border-bottom-color:var(--border-h)}
  .tab.active{color:var(--cyan);border-bottom-color:var(--cyan);text-shadow:0 0 12px rgba(0,212,245,.5)}
  .tab-icon{font-size:.9rem}
  .status-bar{display:flex;align-items:center;gap:12px;margin-left:auto;flex-shrink:0}
  .status-pill{display:flex;align-items:center;gap:6px;background:rgba(0,180,100,.12);border:1px solid rgba(0,180,100,.3);border-radius:20px;padding:4px 12px;font-size:.74rem;color:#4ade80;font-weight:500}
  .status-dot{width:6px;height:6px;border-radius:50%;background:#4ade80;box-shadow:0 0 6px #4ade80}
  @media(max-width:520px){
    .navbar{padding:0 6px}
    .brand{margin-right:8px}
    .brand-text{display:none}
    .tab{padding:0 10px;font-size:.78rem;gap:4px}
    .status-pill .pill-text{display:none}
    .status-pill{padding:4px 8px}
    .tabs{scrollbar-width:thin;scrollbar-color:rgba(0,212,245,.3) transparent}
    .tabs::-webkit-scrollbar{display:block;height:2px}
    .tabs::-webkit-scrollbar-thumb{background:rgba(0,212,245,.3);border-radius:2px}
    .tabs::-webkit-scrollbar-track{background:transparent}
    .row{flex-wrap:wrap;gap:8px}
    .seg{flex-shrink:0}
    .seg-btn{padding:6px 10px;font-size:.75rem}
  }
  .main{position:relative;z-index:1;max-width:960px;margin:0 auto;padding:24px 16px 60px;display:flex;flex-direction:column;gap:16px}
  .page{display:none}.page.active{display:flex;flex-direction:column}
  .card{background:var(--surface);border:1px solid var(--border);border-radius:16px;overflow:hidden;transition:border-color .2s}
  .card:hover{border-color:var(--border-h)}
  .card-header{display:flex;align-items:center;justify-content:space-between;padding:16px 20px;cursor:pointer;user-select:none;border-bottom:1px solid var(--border)}
  .card-header.collapsed{border-bottom-color:transparent}
  .card-title{display:flex;align-items:center;gap:10px;font-size:.95rem;font-weight:600;letter-spacing:.02em}
  .card-icon{width:32px;height:32px;border-radius:9px;display:flex;align-items:center;justify-content:center;font-size:1rem}
  .icon-power{background:rgba(255,77,109,.12);border:1px solid rgba(255,77,109,.25)}
  .icon-pwm{background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.2)}
  .icon-adapt{background:rgba(0,180,170,.1);border:1px solid rgba(0,180,170,.2)}
  .icon-temp{background:rgba(255,159,67,.1);border:1px solid rgba(255,159,67,.2)}
  .icon-sched{background:rgba(0,112,255,.12);border:1px solid rgba(0,112,255,.25)}
  .icon-quick{background:rgba(122,212,255,.1);border:1px solid rgba(122,212,255,.2)}
  .icon-logs{background:rgba(0,180,170,.1);border:1px solid rgba(0,180,170,.2)}
  .chevron{font-size:.7rem;color:var(--text-dim);transition:transform .25s}
  .chevron.open{transform:rotate(180deg)}
  .card-body{padding:20px}
  .card-body.hidden{display:none}
  .rb{padding:5px 11px;background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.1);color:#8a9bb0;border-radius:6px;cursor:pointer;font-size:.78rem;font-weight:500;transition:all .2s}
  .rb:hover{color:#ccc;border-color:rgba(0,212,245,.3)}
  .active-rb{background:rgba(0,212,245,.15)!important;border-color:rgba(0,212,245,.4)!important;color:#00d4f5!important}
  .row{display:flex;align-items:center;justify-content:space-between;padding:12px 0;border-bottom:1px solid rgba(255,255,255,.04);gap:16px}
  .row:last-child{border-bottom:none;padding-bottom:0}
  .row:first-child{padding-top:0}
  .row-label{font-size:.87rem;font-weight:500}
  .row-desc{font-size:.75rem;color:var(--text-dim);margin-top:2px}
  .toggle{position:relative;width:48px;height:26px;flex-shrink:0}
  .toggle input{opacity:0;width:0;height:0}
  .toggle-track{position:absolute;inset:0;background:rgba(255,255,255,.1);border-radius:13px;cursor:pointer;transition:background .25s;border:1px solid rgba(255,255,255,.12)}
  .toggle-thumb{position:absolute;top:3px;left:3px;width:18px;height:18px;border-radius:50%;background:#fff;transition:left .25s;box-shadow:0 1px 4px rgba(0,0,0,.4)}
  .toggle input:checked~.toggle-track{background:var(--cyan);border-color:var(--cyan);box-shadow:var(--glow-c)}
  .toggle input:checked~.toggle-track .toggle-thumb{left:25px}
  .seg{display:flex;border-radius:10px;overflow:hidden;border:1px solid var(--border);background:var(--navy)}
  .seg-btn{padding:6px 14px;font-size:.78rem;font-weight:500;cursor:pointer;color:var(--text-dim);border:none;background:none;transition:all .2s;white-space:nowrap;font-family:'Outfit',sans-serif}
  .seg-btn.active{background:var(--cyan);color:#000;font-weight:600}
  .seg-btn:not(.active):hover{color:var(--text);background:rgba(255,255,255,.05)}
  .slider-wrap{display:flex;flex-direction:column;gap:10px}
  .slider-label-row{display:flex;align-items:center;justify-content:space-between;font-size:.82rem}
  .slider-label{color:var(--text-dim);display:flex;align-items:center;gap:6px}
  .channel-dot{width:9px;height:9px;border-radius:50%}
  .dot-white{background:#e8f4ff;box-shadow:0 0 6px #e8f4ff}
  .dot-fs{background:#7ad4ff;box-shadow:0 0 6px #7ad4ff}
  .dot-fsw{background:#b0e8ff;box-shadow:0 0 6px #b0e8ff}
  .dot-blue{background:#4060ff;box-shadow:0 0 6px #4060ff}
  .dot-red{background:#ff4d6d;box-shadow:0 0 6px #ff4d6d}
  .dot-all{background:linear-gradient(135deg,#fff,#00d4f5)}
  .slider-val{font-family:'Space Mono',monospace;font-size:.82rem;font-weight:700;min-width:80px;text-align:right}
  .pct{color:var(--text-dim);font-size:.72rem;font-weight:400;margin-left:3px}
  input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:6px;border-radius:3px;cursor:pointer;outline:none;background:rgba(255,255,255,.08)}
  input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:var(--cyan);box-shadow:0 0 10px rgba(0,212,245,.6),0 2px 4px rgba(0,0,0,.4);cursor:pointer;transition:transform .15s}
  input[type=range]::-webkit-slider-thumb:hover{transform:scale(1.15)}
  input[type=range]::-moz-range-thumb{width:18px;height:18px;border-radius:50%;border:none;background:var(--cyan);box-shadow:0 0 10px rgba(0,212,245,.6);cursor:pointer}
  .slider-white input[type=range]::-webkit-slider-thumb{background:#e8f4ff;box-shadow:0 0 10px rgba(232,244,255,.5)}
  .slider-fs input[type=range]::-webkit-slider-thumb{background:#7ad4ff;box-shadow:0 0 10px rgba(122,212,255,.5)}
  .slider-fsw input[type=range]::-webkit-slider-thumb{background:#b0e8ff;box-shadow:0 0 10px rgba(176,232,255,.5)}
  .slider-blue input[type=range]::-webkit-slider-thumb{background:#4060ff;box-shadow:0 0 10px rgba(64,96,255,.6)}
  .slider-red input[type=range]::-webkit-slider-thumb{background:#ff4d6d;box-shadow:0 0 10px rgba(255,77,109,.5)}
  .slider-divider{height:1px;background:var(--border);margin:8px 0}
  .temp-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
  .temp-chip{background:var(--surface2);border:1px solid var(--border);border-radius:12px;padding:14px 12px;text-align:center}
  .temp-chip .label{font-size:.72rem;color:var(--text-dim);margin-bottom:6px}
  .temp-chip .value{font-family:'Space Mono',monospace;font-size:1.35rem;font-weight:700}
  .temp-chip .unit{font-size:.8rem;color:var(--text-dim)}
  .temp-chip.plate{border-color:rgba(255,159,67,.2)}
  .temp-chip.plate .value{color:var(--warm)}
  .temp-chip.water{border-color:rgba(0,212,245,.2)}
  .temp-chip.water .value{color:var(--cyan)}
  .lux-bar-wrap{margin-top:6px}
  .lux-bar-bg{height:8px;border-radius:4px;background:rgba(255,255,255,.07);overflow:hidden}
  .lux-bar-fill{height:100%;border-radius:4px;background:linear-gradient(90deg,#0070ff,#00d4f5);transition:width .6s ease}
  .lux-stats{display:flex;justify-content:space-between;margin-top:6px;font-size:.75rem;color:var(--text-dim)}
  .lux-val{font-family:'Space Mono',monospace;color:var(--cyan);font-weight:700;font-size:.9rem}
  .quick-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
  @media(min-width:500px){.quick-grid{grid-template-columns:repeat(3,1fr)}}
  .qa-btn{display:flex;flex-direction:column;align-items:center;gap:7px;padding:16px 10px;border-radius:12px;cursor:pointer;border:1px solid var(--border);background:var(--surface2);font-family:'Outfit',sans-serif;color:var(--text);font-size:.8rem;font-weight:500;text-align:center;transition:all .2s;line-height:1.3}
  .qa-btn:hover{border-color:var(--cyan);background:rgba(0,212,245,.07);color:var(--cyan)}
  .qa-btn .qa-icon{font-size:1.4rem}
  .sched-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  .time-field{display:flex;flex-direction:column;gap:4px}
  .time-field label{font-size:.75rem;color:var(--text-dim)}
  .time-field input[type=time],.time-field input[type=number]{background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 12px;color:var(--text);font-family:'Space Mono',monospace;font-size:.88rem;outline:none;transition:border-color .2s;width:100%}
  .time-field input[type=time]:focus,.time-field input[type=number]:focus{border-color:var(--cyan)}
  .time-sel{display:flex;align-items:center;gap:4px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:4px 8px;width:100%;box-sizing:border-box}
  .time-sel select{background:transparent;border:none;color:var(--text);font-family:'Space Mono',monospace;font-size:.88rem;outline:none;cursor:pointer;-webkit-appearance:none;appearance:none;text-align:center;width:38px}
  .time-sel select:focus{color:var(--cyan)}
  .time-sel select option{background:#001b3a;color:var(--text)}
  .time-sel-sep{color:var(--cyan);font-family:'Space Mono',monospace;font-weight:700;font-size:1rem;line-height:1}
  .badge{display:inline-block;border-radius:6px;padding:2px 8px;font-size:.72rem;font-weight:600}
  .badge-on{background:rgba(0,212,245,.15);color:var(--cyan)}
  .badge-off{background:rgba(255,255,255,.07);color:var(--text-dim)}
  .ip-bar{background:var(--surface);border:1px solid var(--border);border-radius:10px;padding:9px 16px;font-family:'Space Mono',monospace;font-size:.78rem;color:var(--text-dim);display:flex;align-items:center;gap:10px}
  .ip-bar span{color:var(--cyan)}
  .save-btn{display:block;width:100%;margin-top:14px;padding:10px;background:linear-gradient(135deg,var(--teal),var(--cyan));border:none;border-radius:10px;color:#000;font-family:'Outfit',sans-serif;font-size:.88rem;font-weight:700;cursor:pointer;transition:opacity .2s}
  .save-btn:hover{opacity:.85}
  .pump-slot-row{display:flex;gap:10px;align-items:flex-end;margin-bottom:8px}
  .pump-rm{background:rgba(255,77,109,.1);border:1px solid rgba(255,77,109,.25);color:#ff4d6d;border-radius:8px;padding:8px 12px;cursor:pointer;font-size:.9rem;flex-shrink:0}
  .sig{display:inline-flex;align-items:flex-end;gap:2px;height:13px;vertical-align:middle;margin-left:7px}
  .sig i{display:block;width:3px;background:rgba(255,255,255,.15);border-radius:1px}
  .sig i:nth-child(1){height:4px}
  .sig i:nth-child(2){height:7px}
  .sig i:nth-child(3){height:10px}
  .sig i:nth-child(4){height:13px}
  .sig i.on{background:var(--cyan)}
  .wifi-row{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 0;border-bottom:1px solid rgba(255,255,255,.04)}
  .wifi-row:last-child{border-bottom:none}
  .wifi-ssid{font-size:.87rem;font-weight:500;display:flex;align-items:center;flex-wrap:wrap;gap:6px}
  /* TERMINAL PANE */
  #term-pane{display:none;flex-direction:column;height:calc(100vh - 56px);position:relative;z-index:1}
  #term-pane.active{display:flex}
  #quickbar{display:flex;flex-wrap:wrap;gap:5px;padding:7px 13px;background:rgba(6,13,24,.95);border-bottom:1px solid var(--border);flex-shrink:0}
  .qb{background:rgba(0,180,220,.04);border:1px solid var(--border);color:var(--text-dim);padding:4px 11px;border-radius:20px;cursor:pointer;font-size:.74rem;font-weight:500;transition:.15s;white-space:nowrap;font-family:'Outfit',sans-serif}
  .qb:hover{background:rgba(0,212,245,.1);border-color:var(--border-h);color:var(--cyan)}
  .qb.ok{border-color:rgba(74,222,128,.2);color:#5dffa0}
  .qb.ok:hover{background:rgba(74,222,128,.1)}
  .qb.danger{border-color:rgba(255,77,109,.2);color:#ff7080}
  .qb.danger:hover{background:rgba(255,77,109,.1);color:var(--red)}
  #fbar{display:flex;gap:7px;padding:5px 13px;background:rgba(6,13,24,.9);border-bottom:1px solid var(--border);flex-shrink:0}
  #fi{flex:1;background:var(--surface);border:1px solid var(--border);color:var(--text);border-radius:8px;padding:6px 10px;font-size:.82rem;font-family:'Outfit',sans-serif;outline:none;transition:.15s}
  #fi:focus{border-color:var(--cyan)}
  .fb{background:var(--surface2);border:1px solid var(--border);color:var(--text-dim);border-radius:8px;padding:6px 10px;cursor:pointer;font-size:.82rem}
  #term{flex:1;overflow-y:auto;padding:10px 14px;font-family:'Space Mono',monospace;font-size:12px;line-height:1.6;white-space:pre-wrap;word-break:break-all;background:rgba(6,13,24,.85)}
  #tbar2{display:flex;gap:5px;flex-wrap:wrap;padding:6px 13px;background:rgba(6,13,24,.96);border-top:1px solid var(--border);flex-shrink:0;align-items:center}
  .tb2{background:rgba(0,180,220,.05);border:1px solid var(--border);color:var(--text-dim);padding:4px 10px;border-radius:8px;cursor:pointer;font-size:.74rem;transition:.15s;font-family:'Outfit',sans-serif}
  .tb2:hover{color:var(--text);border-color:var(--border-h)}
  .tb2.on{background:rgba(0,212,245,.1);border-color:rgba(0,212,245,.3);color:var(--cyan)}
  #cnt{font-size:.72rem;color:var(--text-dim);margin-left:auto;font-family:'Space Mono',monospace}
  #cbar{display:flex;gap:8px;padding:8px 13px;background:rgba(6,13,24,.96);border-top:1px solid var(--border);flex-shrink:0}
  #ci{flex:1;background:var(--surface);border:1px solid var(--border);color:var(--text);border-radius:8px;padding:7px 12px;font-size:.82rem;font-family:'Space Mono',monospace;outline:none;transition:.15s}
  #ci:focus{border-color:var(--cyan)}
  #sb{background:linear-gradient(135deg,var(--teal),var(--cyan));border:none;color:#000;padding:7px 18px;border-radius:8px;cursor:pointer;font-size:.82rem;font-weight:700;font-family:'Outfit',sans-serif}
  .ts{color:rgba(0,212,245,.4);font-size:10.5px;margin-right:9px;flex-shrink:0;padding-top:2px}
  .lntxt{flex:1;color:rgba(180,220,255,.72)}
  .ok .lntxt{color:#69f0ae}.warn .lntxt{color:#ffca28}.err .lntxt{color:#ff6b80}
  .ln{display:flex;align-items:baseline;padding:1px 0}
  ::-webkit-scrollbar{width:5px;height:5px}
  ::-webkit-scrollbar-track{background:rgba(0,0,0,.2)}
  ::-webkit-scrollbar-thumb{background:rgba(0,180,220,.18);border-radius:3px}
  ::-webkit-scrollbar-thumb:hover{background:rgba(0,212,245,.32)}
  @media(max-width:480px){.temp-grid{grid-template-columns:1fr 1fr}.sched-grid{grid-template-columns:1fr}}
  /* ── ENERGIA CARD CSS ── */
  .en-row{display:flex;align-items:center;justify-content:space-between;padding:9px 0;border-bottom:1px solid rgba(255,255,255,.035)}
  .en-row:last-child{border-bottom:none;padding-bottom:0}
  .en-row:first-child{padding-top:0}
  .en-lbl{font-size:.83rem;font-weight:500}
  .en-val{font-family:'Space Mono',monospace;font-size:.88rem;font-weight:700}
  .en-unit{font-size:.68rem;color:var(--text-dim);margin-left:3px;font-family:'Outfit',sans-serif}
  .en-period-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:14px}
  .epg{background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:11px 12px;text-align:center}
  .epg-lbl{font-size:.64rem;font-family:'Space Mono',monospace;text-transform:uppercase;letter-spacing:.1em;color:var(--text-dim);margin-bottom:5px}
  .epg-val{font-family:'Space Mono',monospace;font-size:1.05rem;font-weight:700}
  .spark{display:flex;align-items:flex-end;gap:2px;height:44px;padding:0 2px}
  .sp-bar{flex:1;border-radius:2px 2px 0 0;min-height:3px;transition:height .5s ease;background:rgba(0,212,245,.25);cursor:pointer}
  .sp-bar:hover{background:var(--cyan)}
  .sp-bar.hi{background:rgba(0,212,245,.5)}
  .mini-bar-wrap{display:flex;align-items:flex-end;gap:3px}
  .mb{flex:1;border-radius:1px 1px 0 0;min-height:2px;background:rgba(0,212,245,.25)}
  .mb.hi{background:rgba(0,212,245,.55)}
  .mb.today{background:var(--cyan)}
  .period-tabs{display:flex;gap:4px;margin-bottom:14px}
  .ptab{padding:4px 12px;border-radius:20px;font-size:.72rem;font-weight:600;cursor:pointer;border:1px solid var(--border);color:var(--text-dim);background:transparent;transition:all .2s;font-family:'Outfit',sans-serif}
  .ptab:hover{color:var(--text);border-color:var(--border-h)}
  .ptab.on{background:rgba(0,212,245,.1);border-color:rgba(0,212,245,.4);color:var(--cyan)}
  .period-panel{display:none}
  .period-panel.on{display:block}
  @media(max-width:620px){.dash-sbs{grid-template-columns:1fr!important}}
  /* ── PHASE ROWS (harmonogram) ── */
  .phase-row{display:flex;align-items:flex-start;gap:10px;padding:9px 10px;border-radius:9px;border:1px solid transparent;border-left-width:3px;transition:all .25s;margin-bottom:5px}
  .phase-row:last-child{margin-bottom:0}
  .phase-row.ph-active{box-shadow:0 2px 16px rgba(0,0,0,.25)}
  .ph-dot{width:8px;height:8px;border-radius:50%;flex-shrink:0;margin-top:5px}
  @keyframes phblink{0%,100%{opacity:1}50%{opacity:.3}}
  .ph-dot.pulse{animation:phblink 1.6s ease-in-out infinite;width:10px;height:10px;margin-top:4px}
  .ph-content{flex:1;min-width:0}
  .ph-name{font-size:.8rem;font-weight:600;line-height:1.2;margin-bottom:3px}
  .ph-sub{font-size:.65rem;color:var(--text-dim);font-family:'DM Mono',monospace;line-height:1.4}
  .ph-wd{color:rgba(0,212,245,.75)}
  .ph-we{color:rgba(255,159,67,.65)}
  .ph-wd-active{color:var(--cyan);font-weight:700}
  .ph-we-active{color:var(--warm);font-weight:700}
  .ph-now-tag{display:inline-block;margin-top:3px;font-size:.58rem;background:rgba(0,212,245,.18);color:var(--cyan);border:1px solid rgba(0,212,245,.4);padding:1px 6px;border-radius:3px;font-weight:700;letter-spacing:.04em}
  .ph-right{display:flex;flex-direction:column;align-items:flex-end;gap:4px;flex-shrink:0}
  .ph-times{font-family:'Space Mono',monospace;font-size:.7rem;font-weight:700;white-space:nowrap;line-height:1.3}
  .ph-times .td{color:var(--text-dim);font-weight:400}
  .ramp-badge{display:inline-flex;align-items:center;gap:3px;padding:3px 9px;border-radius:5px;font-size:.62rem;font-weight:700;white-space:nowrap;letter-spacing:.03em}
  .rb-up{background:linear-gradient(90deg,rgba(255,130,0,.55),rgba(255,210,60,.35));color:#ffd060;border:1px solid rgba(255,170,30,.7);box-shadow:0 0 10px rgba(255,150,0,.3),inset 0 1px 0 rgba(255,255,100,.15);text-shadow:0 0 8px rgba(255,200,0,.8)}
  .rb-down{background:linear-gradient(90deg,rgba(255,160,50,.3),rgba(30,70,220,.55));color:#a0bcff;border:1px solid rgba(60,100,240,.65);box-shadow:0 0 10px rgba(50,90,220,.25),inset 0 1px 0 rgba(150,180,255,.12);text-shadow:0 0 8px rgba(100,150,255,.7)}
  .rb-down-mid{background:linear-gradient(90deg,rgba(100,190,255,.35),rgba(40,100,160,.45));color:#90d4f8;border:1px solid rgba(80,170,240,.55);box-shadow:0 0 8px rgba(60,140,220,.2)}
  .ph-sep{height:1px;background:rgba(255,255,255,.04);margin:0 0 4px}
  /* ── TOAST ── */
  #toast{position:fixed;top:50%;left:50%;transform:translate(-50%,-50%) scale(.8);z-index:9999;background:rgba(12,24,41,.97);border:1px solid var(--cyan);border-radius:14px;padding:16px 28px;font-family:'Outfit',sans-serif;font-size:.95rem;color:var(--text);text-align:center;box-shadow:0 8px 40px rgba(0,0,0,.7),0 0 20px rgba(0,212,245,.15);pointer-events:none;opacity:0;transition:opacity .25s,transform .25s;min-width:200px;max-width:80vw}
  #toast.show{opacity:1;transform:translate(-50%,-50%) scale(1)}
  #toast.toast-ok{border-color:var(--cyan);color:var(--cyan)}
  #toast.toast-err{border-color:var(--red);color:var(--red)}
  #toast.toast-warn{border-color:var(--warm);color:var(--warm)}
  /* ── STATUS STRIP ── */
  .status-strip{display:flex;flex-wrap:wrap;gap:0;padding:0;margin-bottom:10px;background:var(--surface);border:1px solid var(--border);border-radius:12px;overflow:hidden}
  .s-chip{display:flex;align-items:center;gap:5px;padding:7px 14px;font-size:.74rem;font-family:'Space Mono',monospace;border-right:1px solid var(--border);border-bottom:1px solid var(--border)}
  .s-chip:last-child{border-right:none}
  .s-chip .s-label{color:var(--text-dim);font-size:.68rem;margin-right:2px}
  /* ══════════════ TERMINAL PEŁNY ══════════════ */
  #p-terminal{background:var(--bg);overflow:hidden}
  #t-layout{display:flex;height:calc(100vh - 56px);overflow:hidden}
  /* Sidebar */
  #t-sidebar{width:210px;flex-shrink:0;display:flex;flex-direction:column;border-right:1px solid var(--border);background:var(--surface);overflow-y:auto;scrollbar-width:thin;scrollbar-color:var(--border) transparent}
  .tsb-sec{border-bottom:1px solid var(--border);padding:10px 12px}
  .tsb-title{font-size:.62rem;font-weight:700;letter-spacing:.1em;color:var(--text-dim);text-transform:uppercase;margin-bottom:7px}
  /* Category chips */
  .t-chip{display:flex;align-items:center;gap:6px;padding:5px 7px;border-radius:6px;cursor:pointer;font-size:.73rem;font-weight:500;border:1px solid transparent;transition:all .15s;user-select:none;width:100%;background:none;text-align:left;font-family:'Outfit',sans-serif;color:var(--text-dim)}
  .t-chip:hover{background:rgba(255,255,255,.04);color:var(--text)}
  .t-chip.on{background:rgba(0,212,245,.08);border-color:rgba(0,212,245,.22);color:var(--text)}
  .t-cdot{width:7px;height:7px;border-radius:50%;flex-shrink:0}
  .t-clabel{flex:1}
  .t-ccnt{font-family:'Space Mono',monospace;font-size:.63rem;color:var(--text-dim);background:rgba(255,255,255,.06);border-radius:4px;padding:1px 5px;min-width:20px;text-align:center}
  .t-chip.on .t-ccnt{background:rgba(0,212,245,.15);color:var(--cyan)}
  .cd-all{background:linear-gradient(135deg,var(--cyan),var(--blue))}
  .cd-err{background:#ff3b5c;box-shadow:0 0 4px #ff3b5c}
  .cd-warn{background:#ffc400;box-shadow:0 0 4px #ffc400}
  .cd-temp{background:#ff8c00;box-shadow:0 0 4px #ff8c00}
  .cd-led{background:#e8f4ff;box-shadow:0 0 4px #e8f4ff}
  .cd-ramp{background:#0af;box-shadow:0 0 4px #0af}
  .cd-pump{background:#0066ff;box-shadow:0 0 4px #0066ff}
  .cd-adapt{background:#a855f7;box-shadow:0 0 4px #a855f7}
  .cd-ntp{background:#f471b5;box-shadow:0 0 4px #f471b5}
  /* Sidebar search */
  #t-srch{width:100%;background:var(--surface2);border:1px solid var(--border);border-radius:7px;padding:5px 26px 5px 8px;color:var(--text);font-family:'Space Mono',monospace;font-size:.72rem;outline:none;transition:.15s;box-sizing:border-box}
  #t-srch:focus{border-color:var(--cyan)}
  .t-srch-wrap{position:relative}
  .t-srch-clr{position:absolute;right:6px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--text-dim);cursor:pointer;font-size:.75rem;line-height:1;padding:2px}
  .t-srch-clr:hover{color:var(--text)}
  /* Quick cmds sidebar */
  .t-qcmd{display:flex;align-items:center;gap:7px;padding:5px 7px;border-radius:6px;cursor:pointer;font-size:.72rem;color:var(--text-dim);border:1px solid transparent;background:none;width:100%;font-family:'Outfit',sans-serif;text-align:left;transition:all .13s}
  .t-qcmd:hover{background:rgba(255,255,255,.04);color:var(--text);border-color:var(--border)}
  .t-qcmd.t-danger:hover{background:rgba(255,59,92,.08);color:#ff3b5c;border-color:rgba(255,59,92,.25)}
  .t-qcmd.t-ok:hover{background:rgba(0,232,124,.07);color:#00e87c;border-color:rgba(0,232,124,.25)}
  .t-qi{font-size:.9rem;flex-shrink:0}
  /* Status rows sidebar */
  .t-strow{display:flex;justify-content:space-between;align-items:center;font-size:.71rem;padding:2px 0}
  .t-stkey{color:var(--text-dim)}
  .t-stval{font-family:'Space Mono',monospace;font-weight:600;color:var(--text-bright,#e0f0ff)}
  .t-stval.s-on{color:#00e87c}.t-stval.s-off{color:var(--text-dim)}.t-stval.s-auto{color:var(--cyan)}.t-stval.s-manual{color:#ffc400}.t-stval.s-hot{color:#ff8c00}.t-stval.s-crit{color:#ff3b5c;animation:t-blink .6s step-end infinite}
  @keyframes t-blink{0%,100%{opacity:1}50%{opacity:0}}
  /* Main terminal area */
  #t-main{flex:1;display:flex;flex-direction:column;overflow:hidden}
  #t-fbar{display:flex;align-items:center;gap:7px;padding:6px 13px;background:var(--surface);border-bottom:1px solid var(--border);flex-shrink:0}
  .t-flbl{font-size:.7rem;color:var(--text-dim);white-space:nowrap}
  #t-fi{flex:1;background:var(--surface2);border:1px solid var(--border);border-radius:7px;padding:5px 9px;color:var(--text);font-family:'Space Mono',monospace;font-size:.72rem;outline:none;transition:.15s}
  #t-fi:focus{border-color:var(--cyan)}
  #t-fi.t-fi-active{border-color:rgba(255,196,0,.45);background:rgba(255,196,0,.04)}
  .t-fmode{display:flex;border-radius:6px;overflow:hidden;border:1px solid var(--border);flex-shrink:0}
  .t-fm{padding:4px 8px;background:none;border:none;color:var(--text-dim);font-size:.67rem;font-weight:600;cursor:pointer;font-family:'Outfit',sans-serif;white-space:nowrap;transition:.13s}
  .t-fm.on{background:rgba(0,212,245,.14);color:var(--cyan)}
  #t-lcnt{font-family:'Space Mono',monospace;font-size:.67rem;color:var(--text-dim);white-space:nowrap;flex-shrink:0}
  #t-ecnt{font-family:'Space Mono',monospace;font-size:.67rem;color:#ff3b5c;padding:2px 6px;background:rgba(255,59,92,.08);border-radius:4px;display:none;flex-shrink:0}
  #t-ecnt.vis{display:block}
  /* Terminal output */
  #term{flex:1;overflow-y:auto;padding:5px 0;font-family:'Space Mono',monospace;font-size:12px;line-height:1.65;scrollbar-width:thin;scrollbar-color:rgba(0,212,245,.18) transparent;background:rgba(6,13,24,.88)}
  #term::-webkit-scrollbar{width:5px}
  #term::-webkit-scrollbar-thumb{background:rgba(0,212,245,.14);border-radius:3px}
  .ln{display:flex;padding:0 13px;border-left:2px solid transparent;transition:background .1s}
  .ln:hover{background:rgba(255,255,255,.022)}
  .ln.err{border-left-color:#ff3b5c;background:rgba(255,59,92,.04)}
  .ln.warn{border-left-color:#ffc400;background:rgba(255,196,0,.03)}
  .ln.ok{border-left-color:#00e87c;background:rgba(0,232,124,.025)}
  .ln.t-temp{border-left-color:#ff8c00}
  .ln.t-ramp{border-left-color:#0af}
  .ln.t-adapt{border-left-color:#a855f7}
  .ln.t-hi{background:rgba(255,196,0,.13)!important;border-left-color:#ffc400!important}
  .ts{color:rgba(0,212,245,.3);font-size:10.5px;margin-right:9px;flex-shrink:0;padding-top:2px;user-select:none}
  .lntxt{flex:1;word-break:break-all;color:rgba(180,220,255,.72)}
  .ln.err .lntxt{color:#ff8aa0}.ln.warn .lntxt{color:#ffd966}.ln.ok .lntxt{color:#80f0b0}.ln.t-temp .lntxt{color:#ffb366}.ln.t-ramp .lntxt{color:#80e8ff}.ln.t-adapt .lntxt{color:#c084fc}
  .t-hl{background:rgba(255,196,0,.3);color:#fff;border-radius:2px;padding:0 1px}
  /* Toolbar */
  #t-tbar{display:flex;align-items:center;gap:6px;flex-wrap:wrap;padding:5px 13px;background:rgba(6,13,24,.96);border-top:1px solid var(--border);flex-shrink:0}
  .tb2{background:rgba(0,180,220,.05);border:1px solid var(--border);color:var(--text-dim);padding:4px 9px;border-radius:7px;cursor:pointer;font-size:.72rem;transition:.13s;font-family:'Outfit',sans-serif;white-space:nowrap}
  .tb2:hover{color:var(--text);border-color:var(--border-h)}
  .tb2.on{background:rgba(0,212,245,.1);border-color:rgba(0,212,245,.3);color:var(--cyan)}
  #t-cntrr{margin-left:auto;display:flex;align-items:center;gap:6px}
  #t-theme{background:var(--surface2);border:1px solid var(--border);border-radius:6px;padding:3px 7px;color:var(--text-dim);font-size:.67rem;outline:none;cursor:pointer;font-family:'Outfit',sans-serif}
  #cnt{font-size:.72rem;color:var(--text-dim);font-family:'Space Mono',monospace;display:none}
  /* Command input */
  #cbar{display:flex;gap:8px;padding:8px 13px;background:rgba(6,13,24,.97);border-top:1px solid rgba(0,180,255,.08);flex-shrink:0}
  #t-cinput-wrap{position:relative;flex:1;display:flex}
  #t-pfx{position:absolute;left:11px;top:50%;transform:translateY(-50%);font-family:'Space Mono',monospace;font-size:.8rem;color:var(--cyan);pointer-events:none}
  #ci{flex:1;background:var(--surface);border:1px solid var(--border);color:var(--text);border-radius:8px;padding:7px 12px 7px 26px;font-size:.8rem;font-family:'Space Mono',monospace;outline:none;transition:.15s}
  #ci:focus{border-color:var(--cyan);box-shadow:0 0 0 3px rgba(0,170,255,.07)}
  #sb{background:linear-gradient(135deg,var(--teal),var(--cyan));border:none;color:#000;padding:7px 16px;border-radius:8px;cursor:pointer;font-size:.8rem;font-weight:700;font-family:'Outfit',sans-serif;white-space:nowrap}
  /* Scroll hint */
  #t-scroll-hint{position:absolute;bottom:140px;right:20px;background:var(--surface2);border:1px solid var(--border-h,rgba(0,220,255,.3));border-radius:20px;padding:4px 12px;font-size:.7rem;color:var(--cyan);cursor:pointer;display:none;align-items:center;gap:4px;z-index:10;box-shadow:0 4px 16px rgba(0,0,0,.4)}
  #t-scroll-hint.vis{display:flex}
  @media(max-width:700px){
    #t-sidebar{position:fixed;left:-225px;top:56px;height:calc(100vh - 56px);z-index:200;transition:left .28s cubic-bezier(.4,0,.2,1);width:220px;box-shadow:4px 0 28px rgba(0,0,0,.6);border-right:1px solid var(--border-h)}
    #t-sidebar.open{left:0}
    #t-sb-overlay{display:none;position:fixed;inset:0;top:56px;background:rgba(0,0,10,.55);z-index:199;backdrop-filter:blur(2px)}
    #t-sb-overlay.vis{display:block}
    #t-sb-toggle{display:flex!important}
  }
  @media(min-width:701px){#t-sb-toggle{display:none!important}#t-sb-overlay{display:none!important}}
  #t-sb-toggle{display:none;align-items:center;gap:5px;justify-content:center;background:rgba(0,212,245,.08);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:8px;padding:5px 11px;cursor:pointer;font-size:.85rem;font-family:'Outfit',sans-serif;font-weight:600;flex-shrink:0;transition:.15s;white-space:nowrap}
  #t-sb-toggle:hover{background:rgba(0,212,245,.14);border-color:var(--border-h);box-shadow:0 0 8px rgba(0,212,245,.2)}
  #t-sb-toggle.on{background:rgba(0,212,245,.18);border-color:var(--cyan);box-shadow:0 0 10px rgba(0,212,245,.25)}
  #t-sb-toggle .tsb-badge{background:rgba(255,196,0,.85);color:#000;border-radius:9px;padding:1px 5px;font-size:.58rem;font-weight:700;display:none;margin-left:2px}
  #t-sb-toggle .tsb-badge.vis{display:inline}
  @media(max-width:480px){.temp-grid{grid-template-columns:1fr 1fr}.sched-grid{grid-template-columns:1fr}}
  /* ── TOAST ── */
  #toast{position:fixed;top:50%;left:50%;transform:translate(-50%,-50%) scale(.8);z-index:9999;background:rgba(12,24,41,.97);border:1px solid var(--cyan);border-radius:14px;padding:16px 28px;font-family:'Outfit',sans-serif;font-size:.95rem;color:var(--text);text-align:center;box-shadow:0 8px 40px rgba(0,0,0,.7),0 0 20px rgba(0,212,245,.15);pointer-events:none;opacity:0;transition:opacity .25s,transform .25s;min-width:200px;max-width:80vw}
  #toast.show{opacity:1;transform:translate(-50%,-50%) scale(1)}
  #toast.toast-ok{border-color:var(--cyan);color:var(--cyan)}
  #toast.toast-err{border-color:var(--red);color:var(--red)}
  #toast.toast-warn{border-color:var(--warm);color:var(--warm)}
</style>
</head>
<body>
<div id='toast'></div>

<!-- NAVBAR -->
<nav class='navbar'>
  <div class='brand'>
    <div class='brand-dot' id='bdot'></div>
    <span class="brand-text">AKWARIUM LED</span>
  </div>
  <div class='tabs'>
    <div class='tab active' onclick='showTab("dash",this)'><span class='tab-icon'>&#8862;</span> Dashboard</div>
    <div class='tab' onclick='showTab("ustawienia",this)'><span class='tab-icon'>&#9881;</span> Ustawienia</div>
    <div class='tab' onclick='showTab("terminal",this)'><span class='tab-icon'>&#8250;_</span> Terminal</div>
    <div class='tab' onclick='showTab("wykresy",this)'><span class='tab-icon'>&#8599;</span> Wykresy</div>
    <div class='tab' onclick='showTab("logi",this)'><span class='tab-icon'>&#8801;</span> Logi</div>
  </div>
  <div class='status-bar'>
    <div class='status-pill'><div class='status-dot'></div><span class='pill-text'> <span id='connlabel'>Pol&#261;czono</span></span></div>
  </div>
</nav>

<!-- ═══ NAGŁÓWEK SYSTEMOWY (v3 style) ═══ -->
<div id='sys-hdr' style='background:rgba(6,13,24,.95);backdrop-filter:blur(20px);border-bottom:1px solid var(--border);padding:10px 20px;display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap'>
  <div style='display:flex;align-items:center;gap:12px'>
    <div style='font-size:1.6rem;filter:drop-shadow(0 0 8px rgba(0,200,255,.4))'>&#x1F420;</div>
    <div>
      <div style='font-family:Syne,sans-serif;font-size:1.1rem;font-weight:700;color:#fff;letter-spacing:.02em'>Akwarium LED</div>
      <div style='font-family:Space Mono,monospace;font-size:.72rem;color:var(--text-dim);margin-top:2px'><span id='sh-ip' style='color:var(--cyan)'>...</span>&nbsp;&middot;&nbsp;<span id='sh-time'>--:--:--</span></div>
    </div>
  </div>
  <div style='display:flex;gap:6px;flex-wrap:wrap;align-items:center'>
    <div id='sh-wifi' class='sys-badge ok'><div class='sbdot' style='background:#4ade80'></div>WiFi</div>
    <div id='sh-ntp'  class='sys-badge ok'><div class='sbdot' style='background:#4ade80'></div>NTP</div>
    <div id='sh-mode' class='sys-badge blue'>AUTO</div>
  </div>
</div>

<!-- ═══ PAGE: USTAWIENIA ═══ -->
<div class='page main' id='p-ustawienia'>

  <div class='ip-bar'>&#127760; <span id='ip-display'>...</span> &nbsp;/&nbsp; ESP32 Akwarium LED Panel</div>

  <!-- ZASILANIE I TRYB -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-power'>&#9889;</div>Zasilanie i tryb</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div class='row'>
        <div><div class='row-label'>Zasilanie LED</div><div class='row-desc'>W&#322;&#261;cz lub wy&#322;&#261;cz wszystkie diody</div></div>
        <label class='toggle'>
          <input type='checkbox' id='tog-power' onchange='setPower(this.checked)'>
          <div class='toggle-track'><div class='toggle-thumb'></div></div>
        </label>
      </div>
      <div class='row'>
        <div><div class='row-label'>Tryb pracy</div><div class='row-desc'>Auto: harmonogram dzienny / Manual: sta&#322;e warto&#347;ci PWM</div></div>
        <div class='seg'>
          <button class='seg-btn' id='btn-auto' onclick='setTryb(1,this)'>Auto</button>
          <button class='seg-btn' id='btn-man' onclick='setTryb(0,this)'>Manual</button>
        </div>
      </div>
      <div class='row'>
        <div><div class='row-label'>Czas rampy</div><div class='row-desc'>P&#322;ynne przej&#347;cie przy zmianie jasno&#347;ci (minuty)</div></div>
        <div style='display:flex;align-items:center;gap:8px'>
          <input type='number' id='inp-fade' min='1' max='180' value='30' style='width:70px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 10px;color:var(--text);font-family:"Space Mono",monospace;font-size:.85rem;outline:none'>
          <button onclick='setFade()' style='padding:6px 12px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:8px;cursor:pointer;font-family:"Outfit",sans-serif;font-size:.8rem'>Ustaw</button>
        </div>
      </div>
    </div>
  </div>

  <!-- STEROWANIE PWM -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-pwm'>&#9639;</div>Sterowanie PWM <span style='color:var(--text-dim);font-weight:400;font-size:.8rem;margin-left:6px'>(0 &#8211; 1023)</span></div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div class='slider-wrap'>
        <div class='slider-label-row'>
          <div class='slider-label'><div class='channel-dot dot-all'></div> &#11088; Wszystkie kana&#322;y</div>
          <div class='slider-val' id='val-all'>&#8212;</div>
        </div>
        <input type='range' min='0' max='1023' value='0' id='sl-all' oninput='setAll(this)' onchange='setAllSend(this)'>
        <div class='slider-divider'></div>
        <div class='slider-label-row slider-white'><div class='slider-label'><div class='channel-dot dot-white'></div> Bia&#322;e</div><div class='slider-val' id='val-0'>&#8212;</div></div>
        <div class='slider-white'><input type='range' id='sl0' min='0' max='1023' value='0' oninput='updCh(0,this)' onchange='updChSend(0,this)'></div>
        <div class='slider-label-row slider-fs' style='margin-top:10px'><div class='slider-label'><div class='channel-dot dot-fs'></div> Full Spectrum</div><div class='slider-val' id='val-1'>&#8212;</div></div>
        <div class='slider-fs'><input type='range' id='sl1' min='0' max='1023' value='0' oninput='updCh(1,this)' onchange='updChSend(1,this)'></div>
        <div class='slider-label-row slider-fsw' style='margin-top:10px'><div class='slider-label'><div class='channel-dot dot-fsw'></div> FS Bia&#322;e</div><div class='slider-val' id='val-2'>&#8212;</div></div>
        <div class='slider-fsw'><input type='range' id='sl2' min='0' max='1023' value='0' oninput='updCh(2,this)' onchange='updChSend(2,this)'></div>
        <div class='slider-label-row slider-blue' style='margin-top:10px'><div class='slider-label'><div class='channel-dot dot-blue'></div> Niebieskie</div><div class='slider-val' id='val-3'>&#8212;</div></div>
        <div class='slider-blue'><input type='range' id='sl3' min='0' max='1023' value='0' oninput='updCh(3,this)' onchange='updChSend(3,this)'></div>
        <div class='slider-label-row slider-red' style='margin-top:10px'><div class='slider-label'><div class='channel-dot dot-red'></div> Czerwone</div><div class='slider-val' id='val-4'>&#8212;</div></div>
        <div class='slider-red'><input type='range' id='sl4' min='0' max='1023' value='0' oninput='updCh(4,this)' onchange='updChSend(4,this)'></div>
        <div style='margin-top:14px'>
          <button onclick='saveToAuto()' style='width:100%;padding:10px;background:rgba(0,212,245,.12);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:10px;cursor:pointer;font-family:inherit;font-size:.85rem;font-weight:700'>&#11088; Zapisz do AUTO (EEPROM)</button>
        </div>
      </div>
    </div>
  </div>

  <!-- REGULACJA ADAPTACYJNA -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-adapt'>&#127807;</div>Regulacja Adaptacyjna</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div style='font-size:.72rem;font-weight:600;letter-spacing:.08em;color:var(--text-dim);text-transform:uppercase;margin-bottom:8px;padding-bottom:6px;border-bottom:1px solid var(--border)'>Czujnik &#347;wiat&#322;a</div>
      <div class='row'>
        <div><div class='row-label'>Tryb czujnik&#243;w</div><div class='row-desc'>Kt&#243;re czujniki brane pod uwag&#281;</div></div>
        <div class='seg'>
          <button class='seg-btn' id='sa-s0' onclick='setSensor(0)'>OFF</button>
          <button class='seg-btn' id='sa-s1' onclick='setSensor(1)'>Pok&#243;j</button>
          <button class='seg-btn' id='sa-s2' onclick='setSensor(2)'>Pok&#243;j+Woda</button>
        </div>
      </div>
      <div class='row'>
        <div><div class='row-label'>Filtr czujnika (EMA)</div><div class='row-desc'>Szybko&#347;&#263; reakcji na zmian&#281; s&#322;o&#324;ca: 0.05=wolny (odporny na chmury) &#8230; 0.50=szybki</div></div>
        <div style='display:flex;align-items:center;gap:8px'>
          <input type='number' id='inp-ema' min='0.05' max='0.50' step='0.05' value='0.25' style='width:80px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 10px;color:var(--text);font-family:"Space Mono",monospace;font-size:.85rem;outline:none'>
          <button onclick='setEmaFilter()' style='padding:6px 12px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:8px;cursor:pointer;font-family:"Outfit",sans-serif;font-size:.8rem'>Ustaw</button>
        </div>
      </div>
      <div class='row'>
        <div><div class='row-label'>Interwał czujnika (SENS_INT)</div><div class='row-desc'>Co ile sekund odczyt TSL2561 i aktualizacja EMA (5&#8211;120s) &nbsp;&#9654;&nbsp; AutoTune: 5s</div></div>
        <div style='display:flex;align-items:center;gap:8px'>
          <input type='number' id='inp-sens' min='5' max='120' step='5' value='30' style='width:80px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 10px;color:var(--text);font-family:"Space Mono",monospace;font-size:.85rem;outline:none'>
          <button onclick='setSensInt()' style='padding:6px 12px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:8px;cursor:pointer;font-family:"Outfit",sans-serif;font-size:.8rem'>Ustaw</button>
        </div>
      </div>
      <div class='row'>
        <div><div class='row-label'>Czas rampy PWM (RAMP_SEC)</div><div class='row-desc'>Jak d&#322;ugo PWM p&#322;ynnie przechodzi do nowej warto&#347;ci (5&#8211;120s) &nbsp;&#9654;&nbsp; AutoTune: 5s</div></div>
        <div style='display:flex;align-items:center;gap:8px'>
          <input type='number' id='inp-ramp' min='5' max='120' step='5' value='30' style='width:80px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 10px;color:var(--text);font-family:"Space Mono",monospace;font-size:.85rem;outline:none'>
          <button onclick='setRampSec()' style='padding:6px 12px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:var(--cyan);border-radius:8px;cursor:pointer;font-family:"Outfit",sans-serif;font-size:.8rem'>Ustaw</button>
        </div>
      </div>
      <div style='font-size:.72rem;font-weight:600;letter-spacing:.08em;color:var(--text-dim);text-transform:uppercase;margin:14px 0 8px;padding-bottom:6px;border-bottom:1px solid var(--border)'>Adaptacja i MIN LUX</div>
      <div class='row'>
        <div><div class='row-label'>Regulacja adaptacyjna</div><div class='row-desc'>Automatyczne dostosowanie jasno&#347;ci do warunk&#243;w</div></div>
        <label class='toggle'><input type='checkbox' id='tog-adapt'><div class='toggle-track'><div class='toggle-thumb'></div></div></label>
      </div>
      <div class='row'>
        <div><div class='row-label'>Uczenie si&#281;</div><div class='row-desc'>Zbieranie historii LUX do optymalizacji</div></div>
        <label class='toggle'><input type='checkbox' id='tog-learn'><div class='toggle-track'><div class='toggle-thumb'></div></div></label>
      </div>
      <div class='row'>
        <div><div class='row-label'>Min. LUX (mi&#281;dzy fazami)</div><div class='row-desc'>Uzupe&#322;nianie &#347;wiat&#322;a gdy brak aktywnej fazy</div></div>
        <label class='toggle'><input type='checkbox' id='tog-minlux'><div class='toggle-track'><div class='toggle-thumb'></div></div></label>
      </div>
      <div class='row'>
        <div style='width:100%'>
          <div class='row-label'>Docelowy LUX</div>
          <div class='row-desc'>Warto&#347;&#263; LUX utrzymywana w trybie min. LUX</div>
          <div style='display:flex;align-items:center;gap:12px;margin-top:10px'>
            <input type='range' id='sl-lux' min='500' max='8000' step='100' value='2000' oninput='setLuxSlider(this.value)' style='flex:1'>
            <div style='font-family:"Space Mono",monospace;font-size:.9rem;font-weight:700;color:var(--cyan);min-width:64px;text-align:right'><span id='lux-target-val'>2000</span> lx</div>
          </div>
        </div>
      </div>
      <div class='row'>
        <div>
          <div class='row-label'>Interwa&#322; MIN LUX (s)</div>
          <div class='row-desc'>Co ile sekund sprawdza i uzupe&#322;nia lux (10&#8211;300 s)</div>
        </div>
        <div style='display:flex;align-items:center;gap:6px'>
          <input type='number' id='minlux-interval' min='10' max='300' step='5' value='30'
            style='width:72px;background:rgba(0,212,245,.07);border:1px solid rgba(0,212,245,.22);border-radius:8px;color:var(--text);font-size:.88rem;padding:6px 10px;text-align:center'>
          <span style='font-size:.78rem;color:var(--text-dim)'>s</span>
        </div>
      </div>
      <button class='save-btn' onclick='saveAdaptive()'>&#128190; Zapisz regulacj&#281; adaptacyjn&#261;</button>
    </div>
  </div>

  <!-- [v145] SIECI WIFI - w pelni wbudowany panel, bez osobnej podstrony -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-quick'>&#128246;</div>Sieci WiFi</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:10px'>Zapisane sieci</div>
      <div id='wifi-net-list' style='margin-bottom:14px'>
        <div style='font-size:.8rem;color:var(--text-dim)'>&#321;adowanie...</div>
      </div>

      <div class='slider-divider'></div>

      <div style='font-size:.78rem;color:var(--text-dim);margin:12px 0 10px'>Dodaj sie&#263; r&#281;cznie</div>
      <div style='display:grid;gap:10px;margin-bottom:10px'>
        <input id='wifi-new-ssid' type='text' placeholder='SSID'
          style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
        <input id='wifi-new-pass' type='password' placeholder='Has&#322;o'
          style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
      </div>
      <button class='save-btn' onclick='wifiAddNet()'>&#128190; Dodaj sie&#263;</button>

      <div class='slider-divider' style='margin-top:16px'></div>

      <div style='display:flex;align-items:center;justify-content:space-between;margin:12px 0 10px'>
        <div style='font-size:.78rem;color:var(--text-dim)'>Skanuj dost&#281;pne sieci</div>
        <span id='wifi-scan-status' style='font-size:.72rem;color:var(--text-dim)'></span>
      </div>
      <button onclick='wifiStartScan()' style='margin-bottom:10px;background:rgba(0,212,245,.08);border:1px solid rgba(0,212,245,.2);color:var(--cyan);border-radius:8px;padding:8px 16px;font-family:"Outfit",sans-serif;font-size:.82rem;cursor:pointer'>&#128269; Skanuj</button>
      <div id='wifi-scan-list'></div>
    </div>
  </div>

  <!-- TELEGRAM -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-power'>&#128232;</div>Telegram &#8211; powiadomienia</div>
      <span class='chevron'>&#9660;</span>
    </div>
    <div class='card-body hidden'>
      <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:12px'>
        Raport wysyłany automatycznie o 00:00 oraz na komend&#281; /sendlogs w Telegramie.
      </div>
      <div style='display:grid;gap:10px;margin-bottom:12px'>
        <div>
          <label style='font-size:.8rem;color:var(--text-dim);display:block;margin-bottom:4px'>Token bota (od @BotFather)</label>
          <input id='tg-token' type='password' placeholder='123456:ABC-xyz...'
            style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
        </div>
        <div>
          <label style='font-size:.8rem;color:var(--text-dim);display:block;margin-bottom:4px'>Chat ID (Tw&#243;j numer)</label>
          <input id='tg-chatid' placeholder='987654321'
            style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
        </div>
        <div style='display:flex;align-items:center;gap:10px'>
          <label style='font-size:.85rem'>W&#322;&#261;czone:</label>
          <label class='toggle'><input type='checkbox' id='tg-enabled'><div class='toggle-track'><div class='toggle-thumb'></div></div></label>
        </div>
      </div>
      <div style='display:flex;gap:8px;flex-wrap:wrap;margin-bottom:10px'>
        <button class='save-btn' style='flex:0 0 auto' onclick='tgSave()'>&#128190; Zapisz</button>
        <button class='save-btn' style='flex:0 0 auto;background:rgba(255,255,255,.05)' onclick='tgLoad()'>&#128269; Status</button>
      </div>
      <div id='tg-msg' style='font-size:.78rem;min-height:18px;color:#22d3aa'></div>
    </div>
  </div>

  <!-- LOKALIZACJA (zachód słońca) [4.7.0 ASTRO] -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-power'>&#127749;</div>Lokalizacja &#8211; zach&#243;d s&#322;o&#324;ca</div>
      <span class='chevron'>&#9660;</span>
    </div>
    <div class='card-body hidden'>
      <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:12px'>
        Wsp&#243;&#322;rz&#281;dne do wyliczenia zachodu (start wieczornej rampy). Zapisywane na ESP, bez przeflashowania.
      </div>
      <div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:12px'>
        <div>
          <label style='font-size:.8rem;color:var(--text-dim);display:block;margin-bottom:4px'>Szeroko&#347;&#263; (lat, N+)</label>
          <input id='loc-lat' type='number' step='0.0001' min='-90' max='90' placeholder='52.1345'
            style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
        </div>
        <div>
          <label style='font-size:.8rem;color:var(--text-dim);display:block;margin-bottom:4px'>D&#322;ugo&#347;&#263; (lon, E+)</label>
          <input id='loc-lon' type='number' step='0.0001' min='-180' max='180' placeholder='20.1418'
            style='width:100%;box-sizing:border-box;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.15);border-radius:8px;color:var(--text);padding:8px 12px;font-size:.82rem'>
        </div>
      </div>
      <div style='font-size:.82rem;margin-bottom:10px'>Zach&#243;d s&#322;o&#324;ca dzi&#347;: <b id='loc-sunset'>&#8212;</b></div>
      <div style='display:flex;gap:8px;flex-wrap:wrap;margin-bottom:10px'>
        <button class='save-btn' style='flex:0 0 auto' onclick='locSave()'>&#128190; Zapisz</button>
        <button class='save-btn' style='flex:0 0 auto;background:rgba(255,255,255,.05)' onclick='locLoad()'>&#128260; Od&#347;wie&#380;</button>
      </div>
      <div id='loc-msg' style='font-size:.78rem;min-height:18px;color:#22d3aa'></div>
    </div>
  </div>

  <!-- HARMONOGRAM DZIENNY -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-sched'>&#128197;</div>Harmonogram dzienny</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:14px'>&#9728;&#65039; &nbsp;Wsch&#243;d/zach&#243;d s&#322;o&#324;ca &#8212; obliczany automatycznie</div>
      <div class='sched-grid'>
        <div class='time-field'><label>Poranek (powszedni)</label><div class='time-sel' id='ts-mw'></div></div>
        <div class='time-field'><label>Poranek (weekend)</label><div class='time-sel' id='ts-me'></div></div>
        <div class='time-field'><label>Przerwa po&#322;udniowa (OFF)</label><div class='time-sel' id='ts-mo'></div></div>
        <div class='time-field' style='grid-column:1/-1'><label>Wiecz&#243;r &mdash; start przed zachodem s&#322;o&#324;ca</label><div style='display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-top:4px'><span style='font-size:.78rem;color:var(--text-dim)'>Zach&#243;d: <b id='eb-sunset-time' style='color:var(--cyan)'>--:--</b></span><span style='font-size:.78rem;color:var(--text-dim)'>&rarr; LED w&#322;&#261;cza si&#281; o: <b id='eb-result-time' style='color:#f0c040;font-size:.95rem'>--:--</b></span></div><div style='display:flex;align-items:center;gap:10px;margin-top:8px'><span style='font-size:.75rem;color:var(--text-dim);white-space:nowrap'>-180 min</span><input type='range' id='s-eb' min='-180' max='0' step='5' style='flex:1;accent-color:var(--cyan)' oninput='updateEbDisplay()'><span style='font-size:.75rem;color:var(--text-dim);white-space:nowrap'>0 min</span></div><div style='text-align:center;font-size:.8rem;color:var(--text-dim);margin-top:4px'>Offset: <span id='eb-offset-label' style='color:var(--cyan)'>0 min</span></div></div>
        <div class='time-field'><label>Wy&#322;&#261;czenie wieczorne</label><div class='time-sel' id='ts-eo'></div></div>
      </div>
      <button class='save-btn' onclick='saveSched()'>&#128190; Zapisz harmonogram</button>
    </div>
  </div>

  <!-- HARMONOGRAM POMPKI -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-adapt'>&#128166;</div>Harmonogram pompki</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div style='font-size:.8rem;color:var(--text-dim);margin-bottom:12px'>Aktywne okna czasowe (maks. 6)</div>
      <div id='pump-slots'></div>
      <button onclick='addPumpSlot()' style='margin-top:8px;background:rgba(0,212,245,.08);border:1px solid rgba(0,212,245,.2);color:var(--cyan);border-radius:8px;padding:8px 16px;font-family:"Outfit",sans-serif;font-size:.82rem;cursor:pointer'>+ Dodaj przedzia&#322;</button>
      <button class='save-btn' onclick='savePump()'>&#128190; Zapisz pompk&#281;</button>
    </div>
  </div>

  <!-- SZYBKIE AKCJE -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-quick'>&#128640;</div>Szybkie akcje</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div class='quick-grid'>
        <button class='qa-btn' id='btn-pump' onclick='toggleDevice("pump")'><span class='qa-icon'>&#128166;</span><span id='lbl-pump'>Pompa WY&#321;</span></button>
        <button class='qa-btn' id='btn-camera' onclick='toggleDevice("camera")'><span class='qa-icon'>&#128247;</span><span id='lbl-camera'>Kamera WY&#321;</span></button>
        <button class='qa-btn' id='btn-led100' onclick='toggleLed100()'><span class='qa-icon'>&#128161;</span><span id='lbl-led100'>LED 100%</span></button>
        <button class='qa-btn' onclick='qPost("/api/quick/led_off")'><span class='qa-icon'>&#127769;</span>LED WY&#321;</button>
        <button class='qa-btn' onclick='qPost("/api/log-clear")'><span class='qa-icon'>&#129529;</span>Wyczy&#347;&#263; logi</button>
        <a class='qa-btn' href='#' onclick='event.preventDefault();showTab("logi",document.querySelector(".tab:nth-child(4)"))' style='cursor:pointer'><span class='qa-icon'>&#128203;</span>Podgl&#261;d log&#243;w</a>
        <button class='qa-btn' onclick='if(confirm("Restart?"))qPost("/api/quick/restart")' style='border-color:rgba(255,77,109,.3);color:#ff4d6d'><span class='qa-icon'>&#128260;</span>Restart ESP32</button>
      </div>
    </div>
  </div>

  <!-- ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
       [SIM-UI] KARTA SYMULACJI CZUJNIKA ŚWIATŁA
       Aby usunąć: usuń ten blok HTML (od tego komentarza
       do komentarza [SIM-UI] koniec karty)
       ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ -->
  <div class='card' id='sim-card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-adapt'>&#127917;</div>Symulacja czujnika &#347;wiat&#322;a</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <!-- Opis -->
      <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:14px;line-height:1.5'>
        Zastępuje fizyczny czujnik TSL2561 wirtualną wartością lux.<br>
        Przydatne do testowania MIN LUX i adaptacji bez słońca.<br>
        <b style='color:#ff4d6d'>&#9888; Wyłącz przed normalną pracą akwarium!</b>
      </div>

      <!-- Status badge -->
      <div style='margin-bottom:14px'>
        <span id='sim-badge' style='font-size:.8rem;padding:4px 12px;border-radius:20px;font-weight:700'>---</span>
        &nbsp;
        <span id='sim-lux-live' style='font-family:"Space Mono",monospace;font-size:.9rem;color:var(--cyan)'>--- lux</span>
      </div>

      <!-- Przyciski ON / OFF / AUTO -->
      <div style='display:flex;gap:8px;flex-wrap:wrap;margin-bottom:16px'>
        <button onclick='simAction("on")'
          style='padding:9px 20px;border-radius:8px;border:1px solid rgba(0,212,245,.4);
                 background:rgba(0,212,245,.1);color:var(--cyan);font-size:.84rem;cursor:pointer'>
          &#9654; Włącz (stały)
        </button>
        <button onclick='simAction("auto")'
          style='padding:9px 20px;border-radius:8px;border:1px solid rgba(255,179,71,.4);
                 background:rgba(255,179,71,.1);color:#ffb347;font-size:.84rem;cursor:pointer'>
          &#8767; Tryb AUTO (sinusoida)
        </button>
        <button onclick='simAction("off")'
          style='padding:9px 20px;border-radius:8px;border:1px solid rgba(255,77,109,.4);
                 background:rgba(255,77,109,.1);color:#ff4d6d;font-size:.84rem;cursor:pointer'>
          &#9632; Wyłącz (hardware)
        </button>
      </div>

      <!-- Slider stałej wartości lux -->
      <div style='margin-bottom:14px'>
        <label style='font-size:.8rem;color:var(--text-dim)'>Stała wartość lux</label>
        <div style='display:flex;align-items:center;gap:10px;margin-top:6px'>
          <input type='range' id='sim-val-slider' min='0' max='10000' step='50' value='300'
            oninput='document.getElementById("sim-val-num").value=this.value'
            style='flex:1;accent-color:var(--cyan)'>
          <input type='number' id='sim-val-num' min='0' max='50000' value='300'
            oninput='document.getElementById("sim-val-slider").value=Math.min(this.value,10000)'
            style='width:80px;padding:4px 8px;background:var(--surface2);border:1px solid var(--border);
                   color:var(--text);border-radius:6px;font-size:.84rem;text-align:right'>
          <span style='font-size:.8rem;color:var(--text-dim)'>lux</span>
          <button onclick='simSetValue()'
            style='padding:6px 14px;border-radius:7px;border:1px solid rgba(0,212,245,.3);
                   background:rgba(0,212,245,.08);color:var(--cyan);font-size:.8rem;cursor:pointer'>
            Ustaw
          </button>
        </div>
      </div>

      <!-- Parametry AUTO -->
      <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:12px;margin-bottom:10px'>
        <div style='font-size:.78rem;color:var(--text-dim);margin-bottom:10px'>&#8767; Parametry trybu AUTO (sinusoida)</div>
        <div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px'>
          <div>
            <label style='font-size:.75rem;color:var(--text-dim)'>Min lux</label>
            <input type='number' id='sim-auto-min' value='0' min='0' max='50000'
              style='width:100%;margin-top:4px;padding:5px 8px;background:var(--surface);
                     border:1px solid var(--border);color:var(--text);border-radius:6px;font-size:.84rem'>
          </div>
          <div>
            <label style='font-size:.75rem;color:var(--text-dim)'>Max lux</label>
            <input type='number' id='sim-auto-max' value='1500' min='1' max='50000'
              style='width:100%;margin-top:4px;padding:5px 8px;background:var(--surface);
                     border:1px solid var(--border);color:var(--text);border-radius:6px;font-size:.84rem'>
          </div>
          <div>
            <label style='font-size:.75rem;color:var(--text-dim)'>Okres [s]</label>
            <input type='number' id='sim-auto-period' value='600' min='10' max='86400'
              style='width:100%;margin-top:4px;padding:5px 8px;background:var(--surface);
                     border:1px solid var(--border);color:var(--text);border-radius:6px;font-size:.84rem'>
          </div>
        </div>
        <button onclick='simSetAuto()'
          style='margin-top:10px;padding:7px 16px;border-radius:7px;border:1px solid rgba(255,179,71,.3);
                 background:rgba(255,179,71,.08);color:#ffb347;font-size:.8rem;cursor:pointer'>
          Zapisz parametry AUTO
        </button>
      </div>

    </div>
  </div>
  <!-- ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
       [SIM-UI] koniec karty symulacji
       ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ -->

</div><!-- /p-ustawienia -->

<!-- ═══ PAGE: DASHBOARD ═══ -->
<div class='page main active' id='p-dash'>
  <div class='ip-bar'>&#8862; Dashboard &nbsp;&#8212;&nbsp; <span id='dash-time' style='color:var(--cyan)'>--:--:--</span> &nbsp;&#8212;&nbsp; &#128268; <span id='dash-fw' style='color:var(--cyan)'>&#8212;</span></div>

  <!-- STATUS CARD -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-power'>&#9889;</div>Status systemu</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body' style='padding:10px 14px'>
      <div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px'>
        <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>ZASILANIE</div>
          <div id='d-power' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace'>&#8212;</div>
        </div>
        <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>TRYB</div>
          <div id='d-mode' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace'>&#8212;</div>
        </div>
        <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>CZUJNIK</div>
          <div id='d-sensor' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace'>&#8212;</div>
        </div>
        <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>ADAPTACJA</div>
          <div id='d-adapt' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace'>&#8212;</div>
          <div id='d-adapt-trans' style='font-size:.63rem;color:var(--text-dim);margin-top:2px'>&#8212;</div>
        </div>
        <div style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>POMPKA</div>
          <div id='d-pump' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace'>&#8212;</div>
        </div>
        <div id='d-ramp-chip' style='background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:9px 12px'>
          <div style='font-size:.63rem;color:var(--text-dim);letter-spacing:.05em;margin-bottom:3px'>RAMPA</div>
          <div id='d-ramp-inactive' style='font-weight:700;font-size:.82rem;font-family:Space Mono,monospace;color:var(--text-dim)'>&#8212;</div>
          <div id='d-ramp-active' style='display:none'>
            <div style='display:flex;align-items:center;gap:5px'>
              <svg width='32' height='32' viewBox='0 0 32 32' style='flex-shrink:0'>
                <circle cx='16' cy='16' r='12' fill='none' stroke='rgba(255,255,255,.08)' stroke-width='3'/>
                <circle id='d-ramp-arc' cx='16' cy='16' r='12' fill='none'
                  stroke='#00d4f5' stroke-width='3'
                  stroke-linecap='round'
                  stroke-dasharray='75.4 75.4'
                  stroke-dashoffset='75.4'
                  transform='rotate(-90 16 16)'/>
                <text id='d-ramp-pct-txt' x='16' y='20' text-anchor='middle' font-size='7' font-family='Space Mono,monospace' font-weight='700' fill='#00d4f5'>0%</text>
              </svg>
              <div>
                <div id='d-ramp-time' style='font-size:.75rem;font-weight:700;font-family:Space Mono,monospace;color:#00d4f5;line-height:1'>--:--</div>
                <div id='d-ramp-dir' style='font-size:.58rem;color:var(--text-dim);margin-top:2px'>&#9660;</div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- TEMPERATURY -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-temp'>&#127777;</div>Temperatury</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <div class='temp-grid'>
        <div class='temp-chip plate'><div class='label'>P&#322;yta LED 1</div><div class='value' id='d-t1'>&#8212;<span class='unit'>&#176;C</span></div></div>
        <div class='temp-chip plate'><div class='label'>P&#322;yta LED 2</div><div class='value' id='d-t2'>&#8212;<span class='unit'>&#176;C</span></div></div>
        <div class='temp-chip water'><div class='label'>Temperatura wody</div><div class='value' id='d-tw'>&#8212;<span class='unit'>&#176;C</span></div></div>
      </div>
    </div>
  </div>

  <!-- KANAŁY LED & ŚWIATŁO (merged) -->
  <div class='card'>
    <div class='card-header' onclick='toggleCard(this)'>
      <div class='card-title'><div class='card-icon icon-pwm'>&#9728;</div>LED &amp; &#346;wiat&#322;o</div>
      <span class='chevron open'>&#9660;</span>
    </div>
    <div class='card-body'>
      <!-- 3 stat boxes — spójny format: duża cyfra = lx przy roślinach, subtext = skąd -->
      <div style='display:flex;gap:10px;margin-bottom:14px'>

        <!-- SŁOŃCE -->
        <div style='flex:1;background:var(--surface2);border:1px solid rgba(255,159,67,.2);border-radius:12px;padding:12px;text-align:center'>
          <div style='font-size:.65rem;color:var(--text-dim);margin-bottom:2px;letter-spacing:.03em'>&#9728; S&#321;O&#323;CE</div>
          <div style='font-family:\"Space Mono\",monospace;font-size:1.35rem;font-weight:700;color:var(--warm);line-height:1.1' id='d-sun-pct'>&#8212;</div>
          <div style='font-size:.65rem;color:var(--text-dim);margin-top:1px'>lx przy ro&#347;linach</div>
          <div style='font-size:.62rem;color:rgba(255,159,67,.6);margin-top:3px' id='d-sun-lux'>&#8212;</div>
        </div>

        <!-- LED -->
        <div style='flex:1;background:var(--surface2);border:1px solid rgba(0,212,245,.2);border-radius:12px;padding:12px;text-align:center'>
          <div style='font-size:.65rem;color:var(--text-dim);margin-bottom:2px;letter-spacing:.03em'>&#128161; DO&#346;WIETLANIE</div>
          <div style='font-family:\"Space Mono\",monospace;font-size:1.35rem;font-weight:700;color:var(--cyan);line-height:1.1' id='d-led-val'>&#8212;</div>
          <div style='font-size:.65rem;color:var(--text-dim);margin-top:1px'>lx przy ro&#347;linach</div>
          <div style='font-size:.62rem;color:rgba(0,212,245,.5);margin-top:3px' id='d-led-pwm-raw'>&#8212;</div>
        </div>

        <!-- LUX nad wodą -->
        <div style='flex:1;background:var(--surface2);border:1px solid rgba(122,212,255,.2);border-radius:12px;padding:12px;text-align:center'>
          <div style='font-size:.65rem;color:var(--text-dim);margin-bottom:2px;letter-spacing:.03em'>&#127908; NAD WOD&#260;</div>
          <div style='font-family:\"Space Mono\",monospace;font-size:1.35rem;font-weight:700;color:var(--fs);line-height:1.1' id='d-lux-water'>&#8212;</div>
          <div style='font-size:.65rem;color:var(--text-dim);margin-top:1px'>lx (czujnik)</div>
          <div style='font-size:.62rem;color:rgba(122,212,255,.5);margin-top:3px'>bezpo&#347;rednio</div>
        </div>

      </div>
      <!-- Bar + etykieta skali + moc -->
      <div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:4px'>
        <div style='font-size:.68rem;color:var(--text-dim)'>&#347;wiat&#322;o &#322;&#261;cznie przy ro&#347;linach &nbsp;<span id='d-bar-scale' style='color:rgba(255,255,255,.3)'></span></div>
        <div style='font-family:\"Space Mono\",monospace;font-size:.76rem;font-weight:700;color:var(--warm)' id='d-power-now-w'>&#8212;</div>
      </div>
      <div style='position:relative;margin-bottom:6px'>
        <canvas id='d-blend-bar' style='width:100%;height:14px;display:block;border-radius:7px'></canvas>
        <div id='d-bar-sun-pct' style='position:absolute;left:6px;top:50%;transform:translateY(-50%);font-size:10px;font-weight:700;color:#fff;text-shadow:0 0 4px rgba(0,0,0,.9),0 0 8px rgba(0,0,0,.9);pointer-events:none;font-family:Space Mono,monospace'></div>
        <div id='d-bar-pct' style='position:absolute;right:6px;top:50%;transform:translateY(-50%);font-size:10px;font-weight:700;color:#fff;text-shadow:0 0 4px rgba(0,0,0,.9),0 0 8px rgba(0,0,0,.9);pointer-events:none;font-family:Space Mono,monospace'></div>
      </div>
      <!-- Suma usunieta v22 -->
      <!-- Docelowy LUX -->
      <div style='margin-bottom:14px;padding:10px 14px;background:var(--surface2);border:1px solid rgba(0,212,245,.15);border-radius:10px;display:flex;align-items:center;justify-content:space-between'>
        <div>
          <div style='font-size:.65rem;color:var(--text-dim);letter-spacing:.04em;margin-bottom:2px'>&#127919; DOCELOWY LUX <span style='font-size:.6rem'>(tryb MIN. LUX)</span></div>
          <div style='font-size:.72rem;color:var(--text-dim)'>Warto&#347;&#263; utrzymywana gdy brak wystarczaj&#261;cego &#347;wiat&#322;a</div>
        </div>
        <div style='text-align:right;flex-shrink:0;margin-left:12px'>
          <div id='d-minlux-target' style='font-family:Space Mono,monospace;font-size:1.1rem;font-weight:700;color:var(--cyan)'>&#8212;</div>
          <div id='d-minlux-state' style='font-size:.65rem;margin-top:2px'>&#8212;</div>
        </div>
      </div>
      <!-- Separator kanaly -->
      <div style='border-top:1px solid rgba(255,255,255,.06);margin:0 -20px 14px;padding:10px 20px 0;font-size:.72rem;font-weight:600;color:var(--text-dim);letter-spacing:.06em;text-transform:uppercase'>&#9639; Kana&#322;y LED &mdash; PWM / Moc</div>
      <!-- 5 rows with Watts -->
      <div style='display:flex;flex-direction:column;gap:10px'>
        <div style='display:flex;align-items:center;gap:8px'><div style='width:11px;height:11px;border-radius:50%;background:#e8f4ff;box-shadow:0 0 5px #e8f4ff;flex-shrink:0'></div><div style='font-size:.79rem;color:var(--text-dim);width:88px;flex-shrink:0'>Bia&#322;e</div><div style='flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:7px;overflow:hidden'><div id='d-pb0' style='height:100%;border-radius:4px;background:#e8f4ff;transition:width .5s;width:0%'></div></div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--text-dim);min-width:80px;text-align:right;flex-shrink:0' id='d-pv0'>&#8212;</div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--warm);min-width:42px;text-align:right;flex-shrink:0' id='d-pw0'>&#8212;</div></div>
        <div style='display:flex;align-items:center;gap:8px'><div style='width:11px;height:11px;border-radius:50%;background:#7ad4ff;box-shadow:0 0 5px #7ad4ff;flex-shrink:0'></div><div style='font-size:.79rem;color:var(--text-dim);width:88px;flex-shrink:0'>Full Spectrum</div><div style='flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:7px;overflow:hidden'><div id='d-pb1' style='height:100%;border-radius:4px;background:#7ad4ff;transition:width .5s;width:0%'></div></div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--text-dim);min-width:80px;text-align:right;flex-shrink:0' id='d-pv1'>&#8212;</div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--warm);min-width:42px;text-align:right;flex-shrink:0' id='d-pw1'>&#8212;</div></div>
        <div style='display:flex;align-items:center;gap:8px'><div style='width:11px;height:11px;border-radius:50%;background:#b0e8ff;box-shadow:0 0 5px #b0e8ff;flex-shrink:0'></div><div style='font-size:.79rem;color:var(--text-dim);width:88px;flex-shrink:0'>FS Bia&#322;e</div><div style='flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:7px;overflow:hidden'><div id='d-pb2' style='height:100%;border-radius:4px;background:#b0e8ff;transition:width .5s;width:0%'></div></div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--text-dim);min-width:80px;text-align:right;flex-shrink:0' id='d-pv2'>&#8212;</div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--warm);min-width:42px;text-align:right;flex-shrink:0' id='d-pw2'>&#8212;</div></div>
        <div style='display:flex;align-items:center;gap:8px'><div style='width:11px;height:11px;border-radius:50%;background:#4060ff;box-shadow:0 0 5px #4060ff;flex-shrink:0'></div><div style='font-size:.79rem;color:var(--text-dim);width:88px;flex-shrink:0'>Niebieskie</div><div style='flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:7px;overflow:hidden'><div id='d-pb3' style='height:100%;border-radius:4px;background:#4060ff;transition:width .5s;width:0%'></div></div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--text-dim);min-width:80px;text-align:right;flex-shrink:0' id='d-pv3'>&#8212;</div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--warm);min-width:42px;text-align:right;flex-shrink:0' id='d-pw3'>&#8212;</div></div>
        <div style='display:flex;align-items:center;gap:8px'><div style='width:11px;height:11px;border-radius:50%;background:#ff4d6d;box-shadow:0 0 5px #ff4d6d;flex-shrink:0'></div><div style='font-size:.79rem;color:var(--text-dim);width:88px;flex-shrink:0'>Czerwone</div><div style='flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:7px;overflow:hidden'><div id='d-pb4' style='height:100%;border-radius:4px;background:#ff4d6d;transition:width .5s;width:0%'></div></div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--text-dim);min-width:80px;text-align:right;flex-shrink:0' id='d-pv4'>&#8212;</div><div style='font-family:\"Space Mono\",monospace;font-size:.72rem;color:var(--warm);min-width:42px;text-align:right;flex-shrink:0' id='d-pw4'>&#8212;</div></div>
      </div>
    </div>
  </div>

  <!-- HARMONOGRAM DNIA + ENERGIA side-by-side -->
  <div class='dash-sbs' style='display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:12px'>

    <!-- HARMONOGRAM DNIA -->
    <div class='card' style='margin:0'>
      <div class='card-header' style='cursor:default'>
        <div class='card-title'><div class='card-icon icon-sched'>&#128197;</div>Harmonogram dnia</div>
        <span id='d-sched-badge' style='font-size:.73rem;color:var(--text-dim);display:none'>aktywna: <span id='d-sched-phase' style='color:var(--cyan)'></span></span>
      </div>
      <div class='card-body' style='padding:14px 16px'>
        <div style='position:relative;margin-bottom:4px'>
          <canvas id='d-tl-canvas' style='display:block;width:100%;height:14px'></canvas>
          <div id='d-tl-now' style='position:absolute;top:0;width:2px;height:100%;background:rgba(255,255,255,.9);border-radius:1px;box-shadow:0 0 6px #fff;pointer-events:none'></div>
        </div>
        <div style='display:flex;justify-content:space-between;font-size:.6rem;color:var(--text-dim);font-family:Space Mono,monospace;margin-bottom:12px'>
          <span>00:00</span><span>06:00</span><span>12:00</span><span>18:00</span><span>24:00</span>
        </div>
        <div id='d-phase-list' style='display:flex;flex-direction:column;gap:6px'>
          <div style='color:var(--text-dim);font-size:.78rem;text-align:center;padding:10px'>Ladowanie...</div>
        </div>
      </div>
    </div>

    <!-- ENERGIA & CZAS — identyczna z wersja A v3 -->
    <div class='card' style='margin:0'>
      <div class='card-header' style='cursor:default'>
        <div class='card-title'><div class='card-icon icon-adapt'>&#9889;</div>Energia &amp; czas</div>
      </div>
      <div class='card-body' style='padding:14px 16px'>

        <div class='period-tabs'>
          <button class='ptab on' onclick='dSetEPeriod("day",this)'>Dzi&#347;</button>
          <button class='ptab' onclick='dSetEPeriod("week",this)'>Tydzie&#324;</button>
          <button class='ptab' onclick='dSetEPeriod("month",this)'>Miesi&#261;c</button>
        </div>

        <!-- DZIS -->
        <div class='period-panel on' id='dep-day'>
          <div class='en-row'><span class='en-lbl'>Moc teraz</span><span class='en-val' style='color:var(--cyan)'><span id='d-en-pow'>&#8212;</span><span class='en-unit'>W</span></span></div>
          <div class='en-row'><span class='en-lbl'>Energia dzi&#347;</span><span class='en-val' style='color:var(--green)'><span id='d-en-today'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>Koszt dzi&#347;</span><span class='en-val' style='color:var(--warm)'><span id='d-en-cost-today'>&#8212;</span><span class='en-unit'>z&#322;</span></span></div>
          <div class='en-row'><span class='en-lbl'>Czas LED dzi&#347;</span><span class='en-val' style='color:#7ab8ff' id='d-en-ledtime'>&#8212;</span></div>
          <div class='en-row'><span class='en-lbl'>&#9728; Zaoszcz. dzi&#347; (s&#322;o&#324;ce)</span><span class='en-val' style='color:#4ade80'><span id='d-en-saved'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#127807; Aktywacje MIN LUX dzi&#347;</span><span class='en-val' style='color:#a78bfa'><span id='d-en-minlux-act'>&#8212;</span><span class='en-unit'>&times;</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#128161; Lux&middot;godz. przy ro&#347;linach</span><span class='en-val' style='color:#fcd34d'><span id='d-en-luxh'>&#8212;</span><span class='en-unit'>lx&middot;h</span></span></div>
          <div class='en-row' style='border-bottom:none'>
            <span class='en-lbl'>Cena kWh</span>
            <span style='display:flex;align-items:center;gap:6px'>
              <input id='d-kwh-price' type='number' step='0.01' min='0.1' max='5' value='0.80'
                oninput='_kwhPrice=parseFloat(this.value)||_kwhPrice;dUpdatePrice(this.value)'
                onblur='dSaveKwhPrice(this)'
                onkeydown='if(event.key==="Enter"){this.blur();}'
                style='width:64px;background:var(--surface2);border:1px solid var(--border);border-radius:6px;color:var(--text);font-family:Space Mono,monospace;font-size:.82rem;font-weight:700;padding:3px 6px;text-align:right;outline:none;transition:border-color .2s'>
              <span style='font-size:.72rem;color:var(--text-dim)'>z&#322;/kWh</span>
              <span id='d-kwh-saved-icon' style='font-size:.85rem;opacity:0;transition:opacity .3s'>&#10003;</span>
            </span>
          </div>
          <div style='margin-top:10px'>
            <div style='font-size:.72rem;color:var(--text-dim);margin-bottom:6px'>Moc ostatnie 12h</div>
            <div class='spark' id='d-spark-day'></div>
          </div>
        </div>

        <!-- TYDZIEN -->
        <div class='period-panel' id='dep-week'>
          <div class='en-period-grid'>
            <div class='epg'><div class='epg-lbl'>Energia</div><div class='epg-val' id='d-en-week' style='color:var(--green)'>&#8212;</div></div>
            <div class='epg'><div class='epg-lbl'>Koszt</div><div class='epg-val' id='d-en-cost-week' style='color:var(--warm)'>&#8212;</div></div>
            <div class='epg'><div class='epg-lbl'>Czas LED</div><div class='epg-val' id='d-en-ledtime-week' style='color:#7ab8ff'>&#8212;</div></div>
          </div>
          <div style='font-size:.72rem;color:var(--text-dim);margin-bottom:6px'>Energia dzie&#324; po dniu (Wh)</div>
          <div class='mini-bar-wrap' style='height:50px' id='d-week-bars'></div>
          <div style='display:flex;justify-content:space-between;margin-top:4px;font-size:.62rem;color:var(--text-dim);font-family:Space Mono,monospace' id='d-week-labels'></div>
          <div style='height:10px'></div>
          <div class='en-row'><span class='en-lbl'>&#346;r. dzienna energia</span><span class='en-val' style='color:var(--cyan)'><span id='d-en-avg'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#346;r. koszt dzienny</span><span class='en-val' style='color:var(--warm)'><span id='d-en-cost-avg'>&#8212;</span><span class='en-unit'>z&#322;</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#346;r. czas LED</span><span class='en-val' style='color:#7ab8ff' id='d-en-ledtime-avg'>&#8212;</span></div>
          <div class='en-row'><span class='en-lbl'>&#9728; Zaoszcz. tydzie&#324; (s&#322;o&#324;ce)</span><span class='en-val' style='color:#4ade80'><span id='d-en-saved-week'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#127807; &#346;r. aktywacje MIN LUX/dzie&#324;</span><span class='en-val' style='color:#a78bfa'><span id='d-en-minlux-avg'>&#8212;</span><span class='en-unit'>&times;</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#128200; Prognoza koszt/rok</span><span class='en-val' style='color:var(--warm)'><span id='d-en-cost-year'>&#8212;</span><span class='en-unit'>z&#322;</span></span></div>
        </div>

        <!-- MIESIAC -->
        <div class='period-panel' id='dep-month'>
          <div class='en-period-grid'>
            <div class='epg'><div class='epg-lbl'>Energia</div><div class='epg-val' id='d-en-month' style='color:var(--green)'>&#8212;</div></div>
            <div class='epg'><div class='epg-lbl'>Koszt</div><div class='epg-val' id='d-en-cost-month' style='color:var(--warm)'>&#8212;</div></div>
            <div class='epg'><div class='epg-lbl'>Czas LED</div><div class='epg-val' id='d-en-ledtime-month' style='color:#7ab8ff'>&#8212;</div></div>
          </div>
          <div style='font-size:.72rem;color:var(--text-dim);margin-bottom:6px'>Energia tygodniowa (Wh)</div>
          <div class='mini-bar-wrap' style='height:50px' id='d-month-bars'></div>
          <div style='display:flex;justify-content:space-between;margin-top:4px;font-size:.62rem;color:var(--text-dim);font-family:Space Mono,monospace' id='d-month-labels'></div>
          <div class='en-row'><span class='en-lbl'>&#346;r. dzienna energia</span><span class='en-val' style='color:var(--cyan)'><span id='d-en-avg-month'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>Prognoza miesi&#261;c</span><span class='en-val' style='color:var(--text-dim)'><span id='d-en-forecast'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>Prognoza koszt</span><span class='en-val' style='color:var(--warm)'><span id='d-en-cost-forecast'>&#8212;</span><span class='en-unit'>z&#322;</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#9889; Oszcz. vs brak adaptacji</span><span class='en-val' style='color:#4ade80'><span id='d-en-saved-month'>&#8212;</span><span class='en-unit'>z&#322;</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#128161; Energia bez adaptacji</span><span class='en-val' style='color:var(--text-dim)'><span id='d-en-no-adapt'>&#8212;</span><span class='en-unit'>Wh</span></span></div>
          <div class='en-row'><span class='en-lbl'>&#8987; &#322;&#261;czny czas &#347;wiecenia</span><span class='en-val' style='color:#7ab8ff' id='d-en-ledtime-total'>&#8212;</span></div>
          <div class='en-row'><span class='en-lbl'>&#9889; Szczytowa moc</span><span class='en-val' style='color:var(--cyan)'><span id='d-en-peak'>&#8212;</span><span class='en-unit'>W</span></span></div>
        </div>

      </div>
    </div>

  </div><!-- /grid -->

</div><!-- /p-dash -->


<!-- ═══ PAGE: TERMINAL ═══ -->
<div class='page main' id='p-terminal' style='display:none;padding:0'>
<div id='t-layout'>
<div id='t-sb-overlay' onclick='tToggleSidebar()'></div>

  <!-- ═══ SIDEBAR ═══ -->
  <div id='t-sidebar'>

    <div class='tsb-sec'>
      <div class='tsb-title'>Filtry kategorii</div>
      <div id='t-chips'>
        <button class='t-chip on' data-cat='all' onclick='tSetCat("all",this)'><div class='t-cdot cd-all'></div><span class='t-clabel'>Wszystkie</span><span class='t-ccnt' id='tcc-all'>0</span></button>
        <button class='t-chip' data-cat='err' onclick='tSetCat("err",this)'><div class='t-cdot cd-err'></div><span class='t-clabel'>B&#322;&#281;dy</span><span class='t-ccnt' id='tcc-err'>0</span></button>
        <button class='t-chip' data-cat='warn' onclick='tSetCat("warn",this)'><div class='t-cdot cd-warn'></div><span class='t-clabel'>Ostrze&#380;enia</span><span class='t-ccnt' id='tcc-warn'>0</span></button>
        <button class='t-chip' data-cat='temp' onclick='tSetCat("temp",this)'><div class='t-cdot cd-temp'></div><span class='t-clabel'>Temperatury</span><span class='t-ccnt' id='tcc-temp'>0</span></button>
        <button class='t-chip' data-cat='led' onclick='tSetCat("led",this)'><div class='t-cdot cd-led'></div><span class='t-clabel'>LED / PWM</span><span class='t-ccnt' id='tcc-led'>0</span></button>
        <button class='t-chip' data-cat='ramp' onclick='tSetCat("ramp",this)'><div class='t-cdot cd-ramp'></div><span class='t-clabel'>Rampy</span><span class='t-ccnt' id='tcc-ramp'>0</span></button>
        <button class='t-chip' data-cat='pump' onclick='tSetCat("pump",this)'><div class='t-cdot cd-pump'></div><span class='t-clabel'>Pompa</span><span class='t-ccnt' id='tcc-pump'>0</span></button>
        <button class='t-chip' data-cat='adapt' onclick='tSetCat("adapt",this)'><div class='t-cdot cd-adapt'></div><span class='t-clabel'>Adaptacja</span><span class='t-ccnt' id='tcc-adapt'>0</span></button>
        <button class='t-chip' data-cat='ntp' onclick='tSetCat("ntp",this)'><div class='t-cdot cd-ntp'></div><span class='t-clabel'>NTP / czas</span><span class='t-ccnt' id='tcc-ntp'>0</span></button>
      </div>
    </div>

    <div class='tsb-sec'>
      <div class='tsb-title'>Szukaj</div>
      <div class='t-srch-wrap'>
        <input id='t-srch' type='text' placeholder='szukaj w logach&#8230;' oninput='tApplySearch()' onkeydown='if(event.key==="Escape"){tClearSearch()}'>
        <button class='t-srch-clr' onclick='tClearSearch()'>&#10005;</button>
      </div>
    </div>

    <div class='tsb-sec'>
      <div class='tsb-title'>Szybkie komendy</div>
      <button class='t-qcmd' onclick='wscmd("status")'><span class='t-qi'>&#128203;</span> Status systemu</button>
      <button class='t-qcmd' onclick='wscmd("adaptive")'><span class='t-qi'>&#127807;</span> Adaptacja</button>
      <button class='t-qcmd' onclick='wscmd("minlux")'><span class='t-qi'>&#128161;</span> Min Lux</button>
      <button class='t-qcmd' onclick='wscmd("time")'><span class='t-qi'>&#128336;</span> Czas PL</button>
      <button class='t-qcmd' onclick='wscmd("sunset")'><span class='t-qi'>&#127751;</span> Zach&#243;d s&#322;o&#324;ca</button>
      <button class='t-qcmd' onclick='wscmd("ip")'><span class='t-qi'>&#127760;</span> IP urz&#261;dzenia</button>
      <button class='t-qcmd' onclick='wscmd("logsize")'><span class='t-qi'>&#128193;</span> Rozmiar logu</button>
      <button class='t-qcmd' onclick='wscmd("both")'><span class='t-qi'>&#128225;</span> Logi: WiFi+USB</button>
      <button class='t-qcmd' onclick='wscmd("wifi")'><span class='t-qi'>&#128246;</span> Logi: tylko WiFi</button>
      <button class='t-qcmd' onclick='wscmd("serial")'><span class='t-qi'>&#128268;</span> Logi: tylko USB</button>
      <button class='t-qcmd' onclick='wscmd("off")'><span class='t-qi'>&#128263;</span> Logi: wycisz</button>
      <button class='t-qcmd t-ok' onclick='qPost("/api/quick/pump_on")'><span class='t-qi'>&#128166;</span> Pompa W&#321;</button>
      <button class='t-qcmd' onclick='qPost("/api/quick/pump_off")'><span class='t-qi'>&#128166;</span> Pompa WY&#321;</button>
      <button class='t-qcmd t-ok' onclick='qPost("/api/quick/led_test")'><span class='t-qi'>&#128161;</span> LED 100%</button>
      <button class='t-qcmd' onclick='qPost("/api/quick/led_off")'><span class='t-qi'>&#9899;</span> LED WY&#321;</button>
      <button class='t-qcmd t-ok' onclick='qPost("/api/quick/camera_on")'><span class='t-qi'>&#128247;</span> Kamera W&#321;</button>
      <button class='t-qcmd' onclick='qPost("/api/quick/camera_off")'><span class='t-qi'>&#128247;</span> Kamera WY&#321;</button>
      <button class='t-qcmd t-danger' onclick='if(confirm("Restart ESP32?"))qPost("/api/quick/restart")'><span class='t-qi'>&#128260;</span> Restart ESP32</button>
    </div>

    <div class='tsb-sec'>
      <div class='tsb-title'>Status live <span id='t-sage' style='font-size:.6rem;font-weight:400;text-transform:none;color:var(--text-dim)'></span></div>
      <div class='t-strow'><span class='t-stkey'>Zasilanie</span><span class='t-stval' id='tsv-power'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>Tryb</span><span class='t-stval' id='tsv-tryb'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>P&#322;yta 1</span><span class='t-stval' id='tsv-t1'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>P&#322;yta 2</span><span class='t-stval' id='tsv-t2'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>Woda</span><span class='t-stval' id='tsv-tw'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>Lux (pok&#243;j)</span><span class='t-stval' id='tsv-lux'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>PWM&#8320;</span><span class='t-stval' id='tsv-pwm'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>Adaptacja</span><span class='t-stval' id='tsv-adapt'>&#8212;</span></div>
      <div class='t-strow'><span class='t-stkey'>Pompa</span><span class='t-stval' id='tsv-pump'>&#8212;</span></div>
    </div>

  </div><!-- /t-sidebar -->

  <!-- ═══ MAIN TERMINAL AREA ═══ -->
  <div id='t-main'>

    <!-- Filter bar -->
    <div id='t-fbar'>
      <button id='t-sb-toggle' onclick='tToggleSidebar()' title='Filtry i komendy'>&#9776; Filtry<span class='tsb-badge' id='tsb-badge'></span></button>
      <span class='t-flbl'>Filtr:</span>
      <input id='t-fi' type='text' placeholder='s&#322;owo kluczowe&#8230;' oninput='tApplyFilter()' onkeydown='tFilterKey(event)' autocomplete='off'>
      <div class='t-fmode'>
        <button class='t-fm on' id='tfm-text' onclick='tSetFMode("text")'>Tekst</button>
        <button class='t-fm' id='tfm-regex' onclick='tSetFMode("regex")'>Regex</button>
      </div>
      <span id='t-lcnt'>0 linii</span>
      <span id='t-ecnt'>0 b&#322;&#281;d&#243;w</span>
    </div>

    <!-- Output -->
    <div id='term'></div>

    <!-- Toolbar -->
    <div id='t-tbar'>
      <button class='tb2 on' id='asBtn' onclick='toggleAS()'>&#8595; Auto-scroll</button>
      <button class='tb2 on' id='tsBtn' onclick='toggleTS()'>&#128336; Czas</button>
      <button class='tb2 on' id='tColBtn' onclick='tToggleColor()'>&#127912; Kolory</button>
      <button class='tb2' id='tPauseBtn' onclick='tTogglePause()'>&#9208; Pauza</button>
      <button class='tb2' id='tAudBtn' onclick='tToggleAudio()'>&#128277; Alert</button>
      <button class='tb2 on' id='tPingBtn' onclick='tTogglePing()'>&#128241; Ping ON</button>
      <button class='tb2' onclick='tExport()'>&#128190; Eksport</button>
      <button class='tb2' onclick='clearDisp()'>&#128465; Wyczy&#347;&#263;</button>
      <span id='cnt'></span>
      <div id='t-cntrr'>
        <span style='font-size:.67rem;color:var(--text-dim)'>Motyw:</span>
        <select id='t-theme' onchange='tSetTheme(this.value)'>
          <option value=''>Niebieski</option>
          <option value='t-amber'>Bursztynowy</option>
          <option value='t-green'>Zielony</option>
        </select>
      </div>
    </div>

    <!-- Scroll hint -->
    <div id='t-scroll-hint' onclick='tScrollBottom()'>&#8595; nowe logi</div>

    <!-- Command input -->
    <div id='cbar'>
      <div id='t-cinput-wrap'>
        <span id='t-pfx'>&gt;</span>
        <input id='ci' type='text' placeholder='Komenda (help, status, adaptive, restart&#8230;)' onkeydown='ck(event)' autocomplete='off' spellcheck='false'>
      </div>
      <button id='sb' onclick='send()'>Wy&#347;lij &#8629;</button>
    </div>

  </div><!-- /t-main -->
</div><!-- /t-layout -->
</div><!-- /p-terminal -->

<!-- ═══ PAGE: WYKRESY ═══ -->
<div class='page main' id='p-wykresy'>
</div>

<!-- ═══ PAGE: LOGI ═══ -->
<div class='page main' id='p-logi'>
</div>


<script>
var HOST = window.location.hostname;
var PORT = window.location.port || '8080';
var BASE = 'http://' + HOST + ':8080';

// ── ZEGAR LOKALNY ──
(function tick(){
  var n=new Date(),
      h=String(n.getHours()).padStart(2,'0'),
      m=String(n.getMinutes()).padStart(2,'0'),
      s=String(n.getSeconds()).padStart(2,'0');
  var el=document.getElementById('sh-time');
  if(el) el.textContent=h+':'+m+':'+s;
  setTimeout(tick,1000);
})();

// ── TOAST ──
var _toastTimer = null;
function toast(msg, type) {
  var t = document.getElementById('toast');
  if (!t) return;
  t.className = 'show ' + (type==='err'?'toast-err':type==='warn'?'toast-warn':'toast-ok');
  t.textContent = msg;
  clearTimeout(_toastTimer);
  _toastTimer = setTimeout(function(){ t.className=''; }, 2500);
}

// ── IP display ──
document.getElementById('ip-display').textContent = HOST;
  // ── NAGŁÓWEK: IP od razu przy załadowaniu ──
  var shIp = document.getElementById('sh-ip');
  if(shIp) shIp.textContent = HOST;

// ── TAB SWITCHING ──
var PAGES = ['ustawienia','dash','terminal','wykresy','logi'];
var _wykresyBuilt = false;
var _logiBuilt = false;

function buildWykresy() {
  var c = document.getElementById('p-wykresy');
  if (!c) return;
  c.innerHTML =
    "<div class='ip-bar'>&#8599; Wykresy historyczne</div>" +
    "<div style='display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-bottom:10px'>" +
    "<button onclick='loadCharts()' style='padding:7px 13px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:#00d4f5;border-radius:8px;cursor:pointer;font-size:.8rem'>&#8635; Od&#347;wie&#380;</button>" +
    "<button onclick='qPost(\"/api/history/clear\");setTimeout(loadCharts,600)' style='padding:7px 13px;background:rgba(255,77,109,.08);border:1px solid rgba(255,77,109,.2);color:#ff4d6d;border-radius:8px;cursor:pointer;font-size:.8rem'>&#128465; Wyczy&#347;&#263;</button>" +
    "<span style='flex:1'></span>" +
    "<div id='range-btns' style='display:flex;gap:4px'>" +
    "<button onclick='setRange(12)' id='rb-12' class='rb active-rb' title='Ostatnia godzina'>1h</button>" +
    "<button onclick='setRange(36)' id='rb-36' class='rb' title='Ostatnie 3h'>3h</button>" +
    "<button onclick='setRange(72)' id='rb-72' class='rb' title='Ostatnie 6h'>6h</button>" +
    "<button onclick='setRange(0)'  id='rb-0'  class='rb' title='Ostatnie 24h'>24h</button>" +
    "</div></div>" +
    "<div class='card'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-adapt'>&#128200;</div>LUX &mdash; o&#347;wietlenie</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div id='chart-lux-wrap'><svg id='chart-lux' width='100%' height='160' style='display:block'>" +
    "<text x='50%' y='80' text-anchor='middle' fill='#5a7a99' font-size='13'>Kliknij Od&#347;wie&#380;</text></svg></div>" +
    "<div style='display:flex;gap:12px;flex-wrap:wrap;margin-top:6px;font-size:.75rem'>" +
    "<span style='color:#00d4f5'>&#8212; LUX pok&oacute;j</span><span style='color:#ffb347'>&#8212; LUX woda</span></div>" +
    "</div></div>" +
    "<div class='card' style='margin-top:12px'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-temp'>&#127777;</div>Temperatury</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div id='chart-temp-wrap'><svg id='chart-temp' width='100%' height='140' style='display:block'>" +
    "<text x='50%' y='70' text-anchor='middle' fill='#5a7a99' font-size='13'>Kliknij Od&#347;wie&#380;</text></svg></div>" +
    "<div style='display:flex;gap:12px;flex-wrap:wrap;margin-top:6px;font-size:.75rem'>" +
    "<span style='color:#ff9f43'>&#8212; P&#322;yta 1</span><span style='color:#ffd32a'>&#8212; P&#322;yta 2</span><span style='color:#00d4f5'>&#8212; Woda</span></div>" +
    "</div></div>" +
    "<div class='card' style='margin-top:12px'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-pwm'>&#128161;</div>PWM kana&#322;&oacute;w (%)</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div id='chart-pwm-wrap'><svg id='chart-pwm' width='100%' height='140' style='display:block'>" +
    "<text x='50%' y='70' text-anchor='middle' fill='#5a7a99' font-size='13'>Kliknij Od&#347;wie&#380;</text></svg></div>" +
    "<div style='display:flex;gap:12px;flex-wrap:wrap;margin-top:6px;font-size:.75rem'>" +
    "<span style='color:#eee'>&#8212; Bia&#322;e</span><span style='color:#7ab8ff'>&#8212; FS</span>" +
    "<span style='color:#b0e8ff'>&#8212; FS Bia&#322;e</span><span style='color:#448aff'>&#8212; Nieb.</span>" +
    "<span style='color:#ff4d6d'>&#8212; Czerw.</span></div>" +
    "</div></div>" +
    "<div class='card' style='margin-top:12px'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-power'>&#9889;</div>Moc LED [W] w czasie</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div id='chart-power-wrap'><svg id='chart-power' width='100%' height='150' style='display:block'>" +
    "<text x='50%' y='75' text-anchor='middle' fill='#5a7a99' font-size='13'>Kliknij Od&#347;wie&#380;</text></svg></div>" +
    "<div style='display:flex;gap:12px;flex-wrap:wrap;margin-top:6px;font-size:.75rem'>" +
    "<span style='color:#ffd32a'>&#9608; Moc ca&#322;kowita W</span>" +
    "<span id='chart-power-info' style='color:rgba(255,77,109,.7)'></span></div>" +
    "</div></div>" +
    "<div class='card' style='margin-top:12px'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-adapt'>&#127807;</div>Adaptacja &mdash; LUX vs cel</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div id='chart-adapt-wrap'><svg id='chart-adapt' width='100%' height='170' style='display:block'>" +
    "<text x='50%' y='85' text-anchor='middle' fill='#5a7a99' font-size='13'>Kliknij Od&#347;wie&#380;</text></svg></div>" +
    "<div style='display:flex;gap:14px;flex-wrap:wrap;margin-top:6px;font-size:.75rem'>" +
    "<span style='color:#00d4f5'>&#8212; LUX pok&oacute;j</span>" +
    "<span style='color:#4ade80'>&#8213;&#8213; Cel MIN LUX</span>" +
    "<span style='color:#a855f7'>&#9679; MIN LUX aktywny</span>" +
    "<span style='color:rgba(0,212,245,.4)'>&#9646; Adaptacja ON</span></div>" +
    "</div></div>" +
    "<div class='card' style='margin-top:12px'><div class='card-header' onclick='toggleCard(this)'>" +
    "<div class='card-title'><div class='card-icon icon-pwm'>&#127775;</div>Udzia&#322; kana&#322;&oacute;w w mocy (live)</div>" +
    "<span class='chevron open'>&#9660;</span></div><div class='card-body'>" +
    "<div style='display:flex;align-items:center;justify-content:center;flex-wrap:wrap;gap:20px;padding:8px 0'>" +
    "<canvas id='chart-donut' width='190' height='190' style='display:block;flex-shrink:0'></canvas>" +
    "<div id='chart-donut-legend' style='display:flex;flex-direction:column;gap:7px;font-size:.78rem'>" +
    "<span style='color:#5a7a99;font-size:.75rem'>Kliknij Od&#347;wie&#380; moc&#8230;</span></div></div>" +
    "<div style='text-align:center;margin-top:6px'>" +
    "<button onclick='loadDonutLive()' style='padding:5px 14px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.25);color:#00d4f5;border-radius:8px;cursor:pointer;font-size:.75rem'>&#8635; Od&#347;wie&#380; moc</button></div>" +
    "</div></div>" +
    "";
  _wykresyBuilt = true;
  loadCharts();
  loadDonutLive();
}

function buildLogi() {
  var c = document.getElementById('p-logi');
  if (!c) return;
  c.innerHTML =
  "<div class='ip-bar'>&#8801; Logi systemowe</div>" +

  // ══ KARTA 1: Dysk LittleFS — kompaktowy header + tabela plików ══
  "<div class='card'>" +
  "<div class='card-header' onclick='toggleCard(this)'>" +
  "<div class='card-title'><div class='card-icon icon-logs'>&#128451;</div>Dysk LittleFS</div>" +
  "<span class='chevron open'>&#9660;</span></div>" +
  "<div class='card-body' style='padding-top:10px'>" +

  // ─ pasek dysku ─
  "<div style='display:flex;align-items:center;gap:10px;margin-bottom:8px'>" +
  "<div style='flex:1'>" +
  "<div style='background:rgba(255,255,255,.06);border-radius:6px;height:10px;overflow:hidden'>" +
  "<div id='log-bar' style='height:100%;border-radius:6px;width:0%;transition:width .6s;background:linear-gradient(90deg,#00b4aa,#00d4f5)'></div></div></div>" +
  "<div style='white-space:nowrap;font-size:.8rem;font-family:Space Mono,monospace'>" +
  "<span id='log-used' style='color:var(--cyan);font-weight:700'>---</span>" +
  "<span id='log-total' style='color:var(--text-dim)'> / --- MB</span>" +
  "<span id='log-pct' style='margin-left:8px;font-size:.75rem;color:var(--cyan)'>(---%)</span>" +
  "</div>" +
  "<button onclick='loadFsList()' title='Odśwież listę' style='padding:4px 9px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.2);color:#00d4f5;border-radius:7px;cursor:pointer;font-size:.78rem;line-height:1.2'>&#8635;</button>" +
  "</div>" +

  // ─ tabela plików ─
  "<div id='fs-file-list'>" +
  "<div style='color:var(--text-dim);font-size:.78rem;padding:6px 0'>&#8987; &#322;adowanie&#8230;</div>" +
  "</div>" +

  // ─ inline podgląd pliku ─
  "<div id='fs-viewer' style='display:none;margin-top:12px'>" +
  "<div style='display:flex;align-items:center;gap:8px;margin-bottom:6px'>" +
  "<div id='fs-viewer-title' style='flex:1;font-size:.75rem;color:var(--cyan);font-family:Space Mono,monospace'></div>" +
  "<select id='fs-lines' style='background:rgba(0,212,245,.07);border:1px solid rgba(0,212,245,.18);border-radius:7px;color:var(--text);font-size:.75rem;padding:4px 7px'>" +
  "<option value='50'>50 linii</option><option value='100' selected>100 linii</option><option value='500'>500 linii</option><option value='0'>Ca&#322;y</option>" +
  "</select>" +
  "<button onclick='fsReload()' style='padding:4px 10px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.2);color:#00d4f5;border-radius:7px;cursor:pointer;font-size:.75rem'>&#8635;</button>" +
  "<button onclick='document.getElementById(\"fs-viewer\").style.display=\"none\"' style='padding:4px 8px;background:rgba(255,255,255,.06);border:1px solid var(--border);color:var(--text-dim);border-radius:7px;cursor:pointer;font-size:.75rem'>&#10005;</button>" +
  "</div>" +
  "<pre id='fs-content' style='background:rgba(0,0,0,.5);border:1px solid rgba(0,212,245,.12);border-radius:8px;padding:12px;font-family:Space Mono,monospace;font-size:.69rem;color:#b2dfdb;max-height:45vh;overflow-y:auto;white-space:pre-wrap;word-break:break-all;margin:0'></pre>" +
  "</div>" +
  "</div></div>" +

  // ══ KARTA 2: Podgląd log.txt ══
  "<div class='card' style='margin-top:12px'>" +
  "<div class='card-header' onclick='toggleCard(this)'>" +
  "<div class='card-title'><div class='card-icon icon-logs'>&#128203;</div>Podgl&#261;d log.txt</div>" +
  "<span class='chevron open'>&#9660;</span></div>" +
  "<div class='card-body' style='padding-top:10px'>" +

  // ─ kontrolki w jednej linii ─
  "<div style='display:flex;align-items:center;gap:6px;flex-wrap:wrap;margin-bottom:10px'>" +
  "<div style='margin-left:auto;display:flex;gap:6px'>" +
  "<button onclick='loadLogs(0)' style='padding:5px 12px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.22);color:#00d4f5;border-radius:7px;cursor:pointer;font-size:.77rem'>&#8635; Od&#347;wie&#380;</button>" +
  "<button onclick='window.location.href=BASE+\"/api/log-download\"' style='padding:5px 12px;background:rgba(122,212,255,.1);border:1px solid rgba(122,212,255,.22);color:#7ad4ff;border-radius:7px;cursor:pointer;font-size:.77rem'>&#128190; Pobierz</button>" +
  "<button onclick='if(confirm(\"Wyczyścić log.txt?\"))fetch(BASE+\"/api/log-clear\",{method:\"POST\"}).then(function(){loadLogs(0);loadFsList();toast(\"Log wyczyszczony\",\"ok\")})' style='padding:5px 12px;background:rgba(255,77,109,.1);border:1px solid rgba(255,77,109,.28);color:#ff4d6d;border-radius:7px;cursor:pointer;font-size:.77rem'>&#129529; Wyczy&#347;&#263;</button>" +
    "</div></div>" +

  // ─ filtry ─
  "<div id='logi-chips' style='display:flex;flex-wrap:wrap;gap:5px;margin-bottom:8px'>" +
  "<button class='t-chip on' data-lcat='all'  onclick='lSetCat(\"all\",this)'> <div class='t-cdot cd-all'></div> <span class='t-clabel'>Wszystkie</span></button>" +
  "<button class='t-chip'   data-lcat='err'   onclick='lSetCat(\"err\",this)'> <div class='t-cdot cd-err'></div> <span class='t-clabel'>B&#322;&#281;dy</span></button>" +
  "<button class='t-chip'   data-lcat='warn'  onclick='lSetCat(\"warn\",this)'><div class='t-cdot cd-warn'></div><span class='t-clabel'>Ostrze&#380;enia</span></button>" +
  "<button class='t-chip'   data-lcat='temp'  onclick='lSetCat(\"temp\",this)'><div class='t-cdot cd-temp'></div><span class='t-clabel'>Temp</span></button>" +
  "<button class='t-chip'   data-lcat='led'   onclick='lSetCat(\"led\",this)'> <div class='t-cdot cd-led'></div> <span class='t-clabel'>LED/PWM</span></button>" +
  "<button class='t-chip'   data-lcat='ramp'  onclick='lSetCat(\"ramp\",this)'><div class='t-cdot cd-ramp'></div><span class='t-clabel'>Rampy</span></button>" +
  "<button class='t-chip'   data-lcat='pump'  onclick='lSetCat(\"pump\",this)'><div class='t-cdot cd-pump'></div><span class='t-clabel'>Pompa</span></button>" +
  "<button class='t-chip'   data-lcat='adapt' onclick='lSetCat(\"adapt\",this)'><div class='t-cdot cd-adapt'></div><span class='t-clabel'>Adaptacja</span></button>" +
  "<button class='t-chip'   data-lcat='ntp'   onclick='lSetCat(\"ntp\",this)'> <div class='t-cdot cd-ntp'></div> <span class='t-clabel'>NTP</span></button>" +
  "</div>" +

  // ─ szukaj ─
  "<input id='logi-search' type='text' placeholder='&#128269; szukaj w logach&#8230;' oninput='lApplyFilter()' " +
  "style='width:100%;box-sizing:border-box;background:rgba(0,212,245,.06);border:1px solid rgba(0,212,245,.18);border-radius:8px;color:var(--text);font-size:.78rem;padding:7px 12px;font-family:Space Mono,monospace;margin-bottom:10px'>" +

  "<pre id='logs-content' style='background:rgba(0,0,0,.45);border:1px solid rgba(0,212,245,.1);border-radius:8px;padding:13px;font-family:Space Mono,monospace;font-size:.72rem;color:#69f0ae;max-height:60vh;overflow-y:auto;white-space:pre-wrap;word-break:break-all;margin:0'>Kliknij przycisk aby za&#322;adowa&#263; logi&#8230;</pre>" +
  "</div></div>";

  _logiBuilt = true;
  loadFsList();
  loadLogs(0);
}

function showTab(name, el) {
  document.querySelectorAll('.tab').forEach(function(t){ t.classList.remove('active'); });
  if (el) el.classList.add('active');  // [OK] guard: el może być null gdy wywoływane z przycisku
  PAGES.forEach(function(p) {
    var pg = document.getElementById('p-' + p);
    if (pg) pg.style.display = 'none';
  });
  if (name === 'wykresy' && !_wykresyBuilt) buildWykresy();
  if (name === 'logi' && !_logiBuilt) buildLogi();
  var target = document.getElementById('p-' + name);
  if (target) target.style.display = (name === 'terminal') ? 'flex' : 'block';
  if (name === 'ustawienia') loadStatus();
  if (name === 'logi') { loadLogs(0); loadLogStats(); loadFsList(); }
  if (name === 'wykresy' || name === 'dash') loadCharts();
  // [OK] Auto-connect WebSocket gdy wchodzisz na zakładkę Terminal
  if (name === 'terminal') {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
      connect();
    }
    setTimeout(function(){
      var isMobile = /Mobi|Android|iPhone|iPad|iPod/i.test(navigator.userAgent);
      if (!isMobile) { var ci=document.getElementById('ci'); if(ci) ci.focus(); }
    }, 150);
  }
}

// ── GLOBAL CHART DATA ──
var _allChartData = null;
var _chartRange = 12; // domyślnie 1h
var _ledMaxW = 60;

function setRange(n) {
  _chartRange = n;
  document.querySelectorAll('.rb').forEach(function(b){ b.classList.remove('active-rb'); });
  var id = 'rb-' + n;
  var el = document.getElementById(id);
  if (el) el.classList.add('active-rb');
  if (_allChartData) {
    renderCharts(_allChartData);
  } else {
    loadCharts(); // dane nie załadowane - pobierz automatycznie
  }
}

function sliceLast(arr, n) {
  if (!n || arr.length <= n) return arr.slice();
  return arr.slice(arr.length - n);
}

function loadCharts() {
  fetch(BASE+'/api/status').then(function(r){return r.json();}).then(function(d){
    _ledMaxW = d.ledMaxW || 90;
  }).catch(function(){});
  fetch(BASE+'/api/history').then(function(r){return r.text();}).then(function(csv){
    if (!csv || csv.trim().length === 0) {
      ['lux','temp','pwm','power','adapt'].forEach(function(id){
        var s = document.getElementById('chart-'+id);
        if (s) s.innerHTML = '<text x="50%" y="50%" text-anchor="middle" fill="#556" font-size="13">Brak danych historycznych</text>';
      });
      return;
    }
    var rows = csv.trim().split('\n').map(function(r){ return r.split(','); });
    var labels=[], t1=[], t2=[], tw=[], lp=[], lw=[], pw=[[],[],[],[],[]];
    var mlAkt=[], adaptAkt=[], luxCel=[];
    rows.forEach(function(c){
      if (c.length < 11) return;
      if (isNaN(parseFloat(c[1]))) return;
      labels.push((c[0]||'').trim());
      t1.push(parseFloat(c[1])||0);
      t2.push(parseFloat(c[2])||0);
      tw.push(parseFloat(c[3])||0);
      lp.push(parseFloat(c[4])||0);
      lw.push(parseFloat(c[5])||0);
      for (var i = 0; i < 5; i++) pw[i].push(parseInt(c[6+i])||0);
      mlAkt.push(   c.length > 12 && (c[12]||'').trim() === '1' ? 1 : 0);
      adaptAkt.push(c.length > 13 && (c[13]||'').trim() === '1' ? 1 : 0);
      luxCel.push(  c.length > 14 ? (parseFloat(c[14])||0) : 0);
    });
    _allChartData = {
      labels:labels, t1:t1, t2:t2, tw:tw, lp:lp, lw:lw, pw:pw,
      mlAkt:mlAkt, adaptAkt:adaptAkt, luxCel:luxCel
    };
    renderCharts(_allChartData);
  }).catch(function(){ toast('Błąd ładowania historii','err'); });
}

function renderCharts(d) {
  var n  = _chartRange;
  var L  = sliceLast(d.labels,   n);
  var t1 = sliceLast(d.t1,       n);
  var t2 = sliceLast(d.t2,       n);
  var tw = sliceLast(d.tw,       n);
  var lp = sliceLast(d.lp,       n);
  var lw = sliceLast(d.lw,       n);
  var pw = d.pw.map(function(a){ return sliceLast(a, n); });
  var mlAkt   = sliceLast(d.mlAkt    || [], n);
  var adaptAkt= sliceLast(d.adaptAkt || [], n);
  var luxCel  = sliceLast(d.luxCel   || [], n);
  drawChart('chart-lux',  L,
    [{data:lp,color:'#00d4f5'},{data:lw,color:'#ffb347'}], 160);
  drawChart('chart-temp', L,
    [{data:t1,color:'#ff9f43'},{data:t2,color:'#ffd32a'},{data:tw,color:'#00d4f5'}], 140);
  drawChart('chart-pwm',  L,
    [{data:pw[0],color:'#eeeeee'},{data:pw[1],color:'#7ab8ff'},
     {data:pw[2],color:'#b0e8ff'},{data:pw[3],color:'#448aff'},{data:pw[4],color:'#ff4d6d'}], 140);
  var maxW = _ledMaxW || 90;
  var powerW = pw[0].map(function(_, i) {
    var avgPwm = (pw[0][i] + pw[1][i] + pw[2][i] + pw[3][i] + pw[4][i]) / 5.0;
    return Math.round(avgPwm / 100.0 * maxW * 10) / 10;
  });
  drawChartFilled('chart-power', L, powerW, '#ffd32a', 150, maxW);
  var maxPWseen = powerW.length ? Math.max.apply(null, powerW) : 0;
  var piEl = document.getElementById('chart-power-info');
  if (piEl) piEl.textContent = 'Max sesji: ' + maxPWseen.toFixed(1) + ' W  |  limit: ' + maxW + ' W';
  drawChartAdaptacja('chart-adapt', L, lp, luxCel, mlAkt, adaptAkt, 170);
}

// ════════════════════════════════════════════════════════
// NOWE FUNKCJE WYKRESÓW (v29-patch)
// ════════════════════════════════════════════════════════

function drawChartFilled(id, labels, data, color, H, maxScale) {
  var el = document.getElementById(id);
  if (!el) return;
  var W = (el.parentElement ? (el.parentElement.clientWidth || 340) : 340);
  el.setAttribute('width', W);
  el.setAttribute('height', H);
  var PAD = {t:14, r:10, b:28, l:42};
  var cW = W - PAD.l - PAD.r;
  var cH = H - PAD.t - PAD.b;
  var n = labels.length;
  if (n < 2) {
    el.innerHTML = '<text x="50%" y="50%" text-anchor="middle" fill="#556" font-size="13">Brak danych</text>';
    return;
  }
  var mn = 0;
  var mx = (maxScale > 0) ? maxScale : (Math.max.apply(null, data) || 1);
  if (mx === 0) mx = 1;
  var gid = 'gf_' + id.replace(/-/g, '_');
  var svg = '<defs><linearGradient id="' + gid + '" x1="0" x2="0" y1="0" y2="1">' +
    '<stop offset="0%" stop-color="' + color + '" stop-opacity="0.42"/>' +
    '<stop offset="100%" stop-color="' + color + '" stop-opacity="0.03"/>' +
    '</linearGradient></defs>';
  var g, gy, gv;
  for (g = 0; g <= 4; g++) {
    gy = PAD.t + cH - (g / 4) * cH;
    gv = mn + (g / 4) * (mx - mn);
    svg += '<line x1="' + PAD.l + '" y1="' + gy.toFixed(1) + '" x2="' + (W - PAD.r) + '" y2="' + gy.toFixed(1) + '" stroke="rgba(255,255,255,.05)" stroke-width="1"/>';
    svg += '<text x="' + (PAD.l - 4) + '" y="' + (gy + 4).toFixed(1) + '" text-anchor="end" fill="#556" font-size="9">' + gv.toFixed(gv < 10 ? 1 : 0) + '</text>';
  }
  var step = Math.max(1, Math.floor(n / 6));
  var i;
  for (i = 0; i < n; i += step) {
    var xL = PAD.l + (i / (n - 1)) * cW;
    svg += '<text x="' + xL.toFixed(1) + '" y="' + (H - 6) + '" text-anchor="middle" fill="#556" font-size="9">' + (labels[i] || '') + '</text>';
  }
  var pts = '';
  var xi, yi;
  for (i = 0; i < n; i++) {
    xi = (PAD.l + (i / (n - 1)) * cW).toFixed(1);
    yi = (PAD.t + cH - ((data[i] - mn) / (mx - mn)) * cH).toFixed(1);
    pts += xi + ',' + yi + ' ';
  }
  var xFirst = PAD.l.toFixed(1);
  var xLast  = (PAD.l + cW).toFixed(1);
  var yBot   = (PAD.t + cH).toFixed(1);
  svg += '<polygon points="' + xFirst + ',' + yBot + ' ' + pts + xLast + ',' + yBot + '" fill="url(#' + gid + ')" stroke="none"/>';
  svg += '<polyline points="' + pts.trim() + '" fill="none" stroke="' + color + '" stroke-width="2" stroke-linejoin="round"/>';
  if (maxScale > 0) {
    var yLim = (PAD.t + 2).toFixed(1);
    svg += '<line x1="' + PAD.l + '" y1="' + yLim + '" x2="' + (W - PAD.r) + '" y2="' + yLim + '" stroke="rgba(255,77,109,.35)" stroke-width="1.2" stroke-dasharray="5,4"/>';
    svg += '<text x="' + (PAD.l + 4) + '" y="' + (PAD.t + 12) + '" fill="rgba(255,100,100,.55)" font-size="8">' + maxScale + 'W max</text>';
  }
  el.innerHTML = svg;
}

function drawChartAdaptacja(id, labels, luxPokoj, luxCel, mlAkt, adaptAkt, H) {
  var el = document.getElementById(id);
  if (!el) return;
  var W = (el.parentElement ? (el.parentElement.clientWidth || 340) : 340);
  el.setAttribute('width', W);
  el.setAttribute('height', H);
  var PAD = {t:14, r:10, b:28, l:48};
  var cW = W - PAD.l - PAD.r;
  var cH = H - PAD.t - PAD.b;
  var n = labels.length;
  if (n < 2) {
    el.innerHTML = '<text x="50%" y="50%" text-anchor="middle" fill="#556" font-size="13">Brak danych</text>';
    return;
  }
  var mn = 0;
  var allV = luxPokoj.concat(luxCel.filter(function(v){ return v > 0; }));
  var mx = (allV.length ? Math.max.apply(null, allV) : 1) * 1.08;
  if (mx < 50) mx = 50;
  var gidA = 'ga_' + id.replace(/-/g, '_');
  var svg = '<defs><linearGradient id="' + gidA + '" x1="0" x2="0" y1="0" y2="1">' +
    '<stop offset="0%" stop-color="#00d4f5" stop-opacity="0.18"/>' +
    '<stop offset="100%" stop-color="#00d4f5" stop-opacity="0.01"/>' +
    '</linearGradient></defs>';
  var g, gy, gv;
  for (g = 0; g <= 4; g++) {
    gy = PAD.t + cH - (g / 4) * cH;
    gv = mn + (g / 4) * (mx - mn);
    svg += '<line x1="' + PAD.l + '" y1="' + gy.toFixed(1) + '" x2="' + (W - PAD.r) + '" y2="' + gy.toFixed(1) + '" stroke="rgba(255,255,255,.05)" stroke-width="1"/>';
    var gvLabel = gv >= 1000 ? (gv / 1000).toFixed(1) + 'k' : Math.round(gv);
    svg += '<text x="' + (PAD.l - 4) + '" y="' + (gy + 4).toFixed(1) + '" text-anchor="end" fill="#556" font-size="9">' + gvLabel + '</text>';
  }
  var step = Math.max(1, Math.floor(n / 6));
  var i;
  for (i = 0; i < n; i += step) {
    var xL = PAD.l + (i / (n - 1)) * cW;
    svg += '<text x="' + xL.toFixed(1) + '" y="' + (H - 6) + '" text-anchor="middle" fill="#556" font-size="9">' + (labels[i] || '') + '</text>';
  }
  for (i = 0; i < n; i++) {
    if (adaptAkt[i] === 1) {
      var xAd = (PAD.l + (i / (n - 1)) * cW).toFixed(1);
      var barW = Math.max(2, (cW / n)).toFixed(1);
      svg += '<rect x="' + xAd + '" y="' + PAD.t + '" width="' + barW + '" height="' + cH + '" fill="rgba(0,212,245,.06)"/>';
    }
  }
  var ptsLP = '';
  for (i = 0; i < n; i++) {
    var xi = (PAD.l + (i / (n - 1)) * cW).toFixed(1);
    var yi = (PAD.t + cH - ((luxPokoj[i] - mn) / (mx - mn)) * cH).toFixed(1);
    ptsLP += xi + ',' + yi + ' ';
  }
  var xFirst = PAD.l.toFixed(1);
  var xLast  = (PAD.l + cW).toFixed(1);
  var yBot   = (PAD.t + cH).toFixed(1);
  svg += '<polygon points="' + xFirst + ',' + yBot + ' ' + ptsLP + xLast + ',' + yBot + '" fill="url(#' + gidA + ')" stroke="none"/>';
  svg += '<polyline points="' + ptsLP.trim() + '" fill="none" stroke="#00d4f5" stroke-width="2" stroke-linejoin="round"/>';
  var celSeg = '';
  for (i = 0; i < n; i++) {
    if (luxCel[i] > 0) {
      var xc = (PAD.l + (i / (n - 1)) * cW).toFixed(1);
      var yc = (PAD.t + cH - ((luxCel[i] - mn) / (mx - mn)) * cH).toFixed(1);
      celSeg += xc + ',' + yc + ' ';
    }
  }
  if (celSeg.length > 0) {
    svg += '<polyline points="' + celSeg.trim() + '" fill="none" stroke="#4ade80" stroke-width="1.6" stroke-dasharray="6,4" opacity="0.8"/>';
  }
  for (i = 0; i < n; i++) {
    if (mlAkt[i] === 1) {
      var xm = (PAD.l + (i / (n - 1)) * cW).toFixed(1);
      svg += '<circle cx="' + xm + '" cy="' + (PAD.t + cH - 4) + '" r="3.5" fill="#a855f7" opacity="0.85"/>';
    }
  }
  el.innerHTML = svg;
}

function loadDonutLive() {
  fetch(BASE + '/api/status').then(function(r){ return r.json(); }).then(function(d){
    var chW = d.powerNowCh || [0, 0, 0, 0, 0];
    var total = 0, i;
    for (i = 0; i < 5; i++) total += parseFloat(chW[i] || 0);
    var names  = ['Białe', 'Full Spec.', 'FS Białe', 'Niebieskie', 'Czerwone'];
    var colors = ['#e8f4ff', '#7ab8ff', '#b0e8ff', '#4060ff', '#ff4d6d'];
    drawDonutChart(chW, names, colors, total);
  }).catch(function(){ });
}

function drawDonutChart(vals, names, colors, total) {
  var canvas = document.getElementById('chart-donut');
  var legend = document.getElementById('chart-donut-legend');
  if (!canvas) return;
  var ctx = canvas.getContext('2d');
  var W = canvas.width, H = canvas.height;
  var cx = W / 2, cy = H / 2;
  var R = Math.min(cx, cy) * 0.84;
  var r = R * 0.50;
  ctx.clearRect(0, 0, W, H);
  if (total < 0.01) {
    ctx.fillStyle = 'rgba(255,255,255,.07)';
    ctx.beginPath(); ctx.arc(cx, cy, R, 0, 2 * Math.PI); ctx.fill();
    ctx.fillStyle = 'rgba(0,212,245,.5)';
    ctx.font = '12px sans-serif'; ctx.textAlign = 'center';
    ctx.fillText('LED wyłączone', cx, cy + 4);
    if (legend) legend.innerHTML = '<span style="color:#5a7a99;font-size:.75rem">LED off &ndash; brak mocy</span>';
    return;
  }
  var angle = -Math.PI / 2;
  var i, v, slice;
  for (i = 0; i < vals.length; i++) {
    v = parseFloat(vals[i] || 0);
    if (v <= 0) continue;
    slice = (v / total) * 2 * Math.PI;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.arc(cx, cy, R, angle, angle + slice);
    ctx.closePath();
    ctx.fillStyle = colors[i];
    ctx.globalAlpha = 0.92;
    ctx.fill();
    ctx.globalAlpha = 1;
    ctx.strokeStyle = '#060d18';
    ctx.lineWidth = 1.5;
    ctx.stroke();
    angle += slice;
  }
  ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI);
  ctx.fillStyle = '#060d18'; ctx.fill();
  ctx.fillStyle = '#00d4f5'; ctx.font = 'bold 15px monospace'; ctx.textAlign = 'center';
  ctx.fillText(total.toFixed(1) + ' W', cx, cy + 4);
  ctx.fillStyle = 'rgba(255,255,255,.35)'; ctx.font = '9px sans-serif';
  ctx.fillText('łącznie', cx, cy + 16);
  if (!legend) return;
  var html = '';
  for (i = 0; i < vals.length; i++) {
    v = parseFloat(vals[i] || 0);
    var pct = total > 0 ? Math.round(v / total * 100) : 0;
    html += '<div style="display:flex;align-items:center;gap:7px">' +
      '<div style="width:10px;height:10px;border-radius:50%;background:' + colors[i] + ';flex-shrink:0"></div>' +
      '<span style="color:rgba(255,255,255,.6);min-width:76px;font-size:.76rem">' + names[i] + '</span>' +
      '<span style="font-family:monospace;color:' + colors[i] + ';font-weight:700;min-width:36px">' + v.toFixed(1) + 'W</span>' +
      '<span style="color:rgba(255,255,255,.28);font-size:.68rem">(' + pct + '%)</span>' +
      '</div>';
  }
  legend.innerHTML = html;
}

// ════════════════════════════════════════════════════════

function avg(arr) { return arr.length ? arr.reduce(function(a,b){return a+b;},0)/arr.length : 0; }
function fmin(arr) { return arr.length ? Math.min.apply(null,arr) : 0; }
function fmax(arr) { return arr.length ? Math.max.apply(null,arr) : 0; }

// ── kwhPrice cache (update z loadDash) ──
var _kwhPrice = 0.80;

function renderStats(full, t1, t2, tw, lp, lw, pw) {
  var sp = document.getElementById('stats-panel');
  if (!sp) return;
  var allPw = full.pw;
  var n24 = full.labels.length;
  var hoursData = Math.round(n24*5/60*10)/10;
  var avgLuxW = Math.round(avg(lw));
  var maxLuxW = Math.round(fmax(lw));
  var avgTw = avg(tw).toFixed(1);
  var maxT = Math.max(fmax(t1),fmax(t2)).toFixed(1);

  function statBox(icon,label,val,unit,color) {
    return "<div style='background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.06);border-radius:10px;padding:12px 8px;text-align:center'>" +
      "<div style='font-size:1.1rem'>" + icon + "</div>" +
      "<div style='font-size:1.05rem;font-weight:700;color:"+color+";margin:4px 0'>" + val +
      "<span style='font-size:.62rem;color:#445;margin-left:2px'>" + unit + "</span></div>" +
      "<div style='font-size:.63rem;color:#445;line-height:1.2'>" + label + "</div></div>";
  }

  sp.innerHTML =
    "<div style='font-size:.72rem;color:#445;margin-bottom:10px;text-align:left'>Zakres wykresu &bull; Pełne 24h: "+hoursData+"h danych ("+n24+" pomiarów)</div>" +
    "<div style='display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:16px'>" +
    statBox('💧','Śr. temp wody', avgTw,'°C','#00d4f5') +
    statBox('🌡️','Max temp płyty', maxT,'°C','#ff9f43') +
    statBox('☀️','Śr. LUX woda', avgLuxW,'lx','#ffb347') +
    "</div>" +
    "<div id='adapt-stats-live'><div style='color:#445;text-align:center;padding:8px'>Ładowanie statystyk adaptacji...</div></div>" +
    "<div id='energy-stats-live'><div style='color:#445;text-align:center;padding:8px'>Ładowanie danych energii...</div></div>";

  // Wczytaj dane live z /api/status i dorysuj resztę
  fetch(BASE+'/api/status').then(function(r){return r.json();}).then(function(d){
    // Nie nadpisuj _kwhPrice z ESP - priorytet ma localStorage/user
    (function(){
      var lv=null; try{lv=localStorage.getItem('kwhPrice');}catch(e){}
      _kwhPrice = lv ? parseFloat(lv) : (d.kwhPrice || 0.80);
    })();
    var todayWh   = d.energyTodayWh || 0;
    var weekWh    = d.energyWeekWh  || 0;
    var monthWh   = d.energyMonthWh || 0;
    var ledMin    = d.ledOnMinutesToday || 0;
    var luxH      = d.luxHoursTodayWater || 0;
    var mlActiv   = d.minLuxActivToday || 0;
    var price     = _kwhPrice;

    // Oszczędność = średnia redukcja PWM dzięki słońcu (harm.PWM -> adapt.PWM)
    // czyli ile % LEDy zrobiły mniej niż żądał harmonogram, bo słońce uzupełniło resztę
    var sunSavingsPct = parseFloat(d.statSredniaRedukcja) || 0;

    // Kafelki adaptacji
    var korMin  = d.statKorektyMin  || 0;
    var korMax  = d.statKorektyMax  || 0;

    var adaptEl = document.getElementById('adapt-stats-live');
    if (adaptEl) {
      adaptEl.innerHTML =
        "<div style='margin-bottom:14px'>" +
          "<div style='font-size:.72rem;color:#556;font-weight:600;margin-bottom:8px'>🌿 Adaptacja do światła naturalnego</div>" +
          "<div style='display:grid;grid-template-columns:repeat(3,1fr);gap:8px'>" +
            statBox('☀️','Redukcja LEDów przez słońce', sunSavingsPct.toFixed(1),'%','#69f0ae') +
            statBox('🔼','Korekty MIN (zbyt ciemno)', korMin,'×','#ffb347') +
            statBox('🔽','Korekty MAX (zbyt jasno)', korMax,'×','#ff6b6b') +
          "</div>" +
        "</div>";
    }

    function pln(wh){ return (wh/1000*price).toFixed(2); }
    function energyRow(label, wh) {
      return "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-bottom:6px;align-items:center'>" +
        "<div style='font-size:.72rem;color:#667'>" + label + "</div>" +
        "<div style='font-family:Space Mono,monospace;font-size:.82rem;font-weight:700;color:#ffd32a;text-align:center'>" + wh.toFixed(0) + " Wh</div>" +
        "<div style='font-family:Space Mono,monospace;font-size:.82rem;font-weight:700;color:#69f0ae;text-align:right'>" + pln(wh) + " zł</div>" +
      "</div>";
    }

    var ledH = Math.floor(ledMin/60);
    var ledM = ledMin % 60;
    var ledTimeStr = ledH + "h " + String(ledM).padStart(2,'0') + "m";

    var eHtml =
      "<div style='margin-bottom:14px'>" +
        "<div style='display:flex;align-items:center;justify-content:space-between;margin-bottom:8px'>" +
          "<div style='font-size:.72rem;color:#556;font-weight:600'>⚡ Energetyka (max "+_ledMaxW+"W)</div>" +
          "<div style='display:flex;align-items:center;gap:6px'>" +
            "<span style='font-size:.65rem;color:#445'>PLN/kWh:</span>" +
            "<input id='kwh-inp' type='number' value='"+price.toFixed(2)+"' step='0.01' min='0.01' max='99' " +
              "style='width:54px;background:rgba(0,212,245,.08);border:1px solid rgba(0,212,245,.3);border-radius:6px;color:#00d4f5;font-size:.75rem;padding:3px 6px;font-family:Space Mono,monospace;text-align:right' " +
              "onchange='saveKwhPrice(this.value)'>" +
          "</div>" +
        "</div>" +
        "<div style='background:rgba(255,255,255,.03);border-radius:10px;padding:10px 12px'>" +
          "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-bottom:6px'>" +
            "<div></div>" +
            "<div style='font-size:.6rem;color:#445;text-align:center'>ZUŻYCIE</div>" +
            "<div style='font-size:.6rem;color:#445;text-align:right'>KOSZT</div>" +
          "</div>" +
          energyRow("Dzisiaj", todayWh) +
          energyRow("Ten tydzień", weekWh) +
          energyRow("Ten miesiąc", monthWh) +
        "</div>" +
      "</div>" +
      "<div style='display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px'>" +
        statBox('💡','Czas pracy LED dziś', ledTimeStr,'','#00d4f5') +
        statBox('🌊','Lux·h woda dziś', luxH.toFixed(0),'lx·h','#7ab8ff') +
        statBox('🎯','Akt. MIN LUX dziś', mlActiv,'razy','#a855f7') +
      "</div>";

    var eEl = document.getElementById('energy-stats-live');
    if (eEl) eEl.innerHTML = eHtml;

  }).catch(function(){});
}

function saveKwhPrice(v) {
  var val = parseFloat(v);
  if (isNaN(val) || val < 0.01 || val > 99) return;
  fetch(BASE+'/set-kwh-price?v='+val.toFixed(2)).then(function(){
    toast('Cena kWh: '+val.toFixed(2)+' PLN - zapisano');
  }).catch(function(){});
}

function drawChart(id, labels, series, H) {
  var el = document.getElementById(id);
  if (!el) return;
  var W = el.parentElement.clientWidth || 340;
  el.setAttribute('width', W);
  el.setAttribute('height', H);
  var PAD = {t:10,r:8,b:28,l:38};
  var cW = W - PAD.l - PAD.r;
  var cH = H - PAD.t - PAD.b;
  var n = labels.length;
  if (n < 1) { el.innerHTML='<text x="50%" y="50%" text-anchor="middle" fill="#556" font-size="13">Brak danych</text>'; return; }
  if (n === 1) { labels.push(labels[0]); series.forEach(function(s){s.data.push(s.data[0]);}); n=2; }
  // Find global min/max
  var allVals = []; series.forEach(function(s){allVals=allVals.concat(s.data);});
  var mn = Math.min.apply(null,allVals), mx = Math.max.apply(null,allVals);
  if (mx === mn) { mn = mn - 1; mx = mx + 1; }
  var svg = '<rect x="0" y="0" width="'+W+'" height="'+H+'" fill="transparent"/>';
  // Grid lines
  for (var g=0;g<=4;g++) {
    var gy = PAD.t + cH - (g/4)*cH;
    var gv = mn + (g/4)*(mx-mn);
    svg += '<line x1="'+PAD.l+'" y1="'+gy+'" x2="'+(W-PAD.r)+'" y2="'+gy+'" stroke="rgba(255,255,255,.05)" stroke-width="1"/>';
    svg += '<text x="'+(PAD.l-4)+'" y="'+(gy+4)+'" text-anchor="end" fill="#445" font-size="9">'+gv.toFixed(gv<10?1:0)+'</text>';
  }
  // X labels (show ~6 evenly)
  var step = Math.max(1, Math.floor(n/6));
  for (var i=0;i<n;i+=step) {
    var x = PAD.l + (i/(n-1))*cW;
    svg += '<text x="'+x+'" y="'+(H-6)+'" text-anchor="middle" fill="#445" font-size="9">'+labels[i]+'</text>';
  }
  // Series lines
  series.forEach(function(s) {
    var pts = s.data.map(function(v,i){
      var x = PAD.l + (i/(n-1))*cW;
      var y = PAD.t + cH - ((v-mn)/(mx-mn))*cH;
      return x.toFixed(1)+','+y.toFixed(1);
    }).join(' ');
    svg += '<polyline points="'+pts+'" fill="none" stroke="'+s.color+'" stroke-width="1.5" stroke-linejoin="round"/>';
  });
  el.innerHTML = svg;
}

// ── CARD TOGGLE ──
function toggleCard(h) {
  var b = h.nextElementSibling, c = h.querySelector('.chevron');
  if (b.classList.contains('hidden')) {
    b.classList.remove('hidden'); c.classList.add('open'); h.classList.remove('collapsed');
  } else {
    b.classList.add('hidden'); c.classList.remove('open'); h.classList.add('collapsed');
  }
}

// ── SLIDER FILL ──
function updFill(el) {
  var f = parseInt(el.value) / 1023 * 100;
  el.style.background = 'linear-gradient(to right,rgba(0,212,245,.6) 0%,rgba(0,112,255,.4) ' + f + '%,rgba(255,255,255,.08) ' + f + '%)';
}
function setValLabel(id, v) {
  var el = document.getElementById(id);
  if (!el) return;
  el.innerHTML = v + ' <span class="pct">' + Math.round(v/1023*100) + '%</span>';
}

// ── PWM ──
var _pt = null;
function sendPwm() {
  clearTimeout(_pt); _pt = setTimeout(function(){
    var p=''; for(var i=0;i<5;i++) p+=(p?'&':'')+'pwm'+i+'='+document.getElementById('sl'+i).value;
    fetch(BASE+'/set-pwm?'+p).catch(function(){});
  }, 50);
}
// oninput = only update labels/fill visually (no send)
function updCh(ch, el) {
  var v = el.value;
  setValLabel('val-'+ch, v); updFill(el);
  var s=0; for(var i=0;i<5;i++) s+=parseInt(document.getElementById('sl'+i).value);
  var a=Math.round(s/5);
  setValLabel('val-all', a);
  document.getElementById('sl-all').value = a; updFill(document.getElementById('sl-all'));
}
// onchange = fires on release -> sends to ESP -> 10s ramp kicks in
function updChSend(ch, el) {
  updCh(ch, el);
  sendPwm();
}
function setAll(el) {
  var v = el.value; setValLabel('val-all', v); updFill(el);
  for(var i=0;i<5;i++){
    document.getElementById('sl'+i).value=v;
    setValLabel('val-'+i,v); updFill(document.getElementById('sl'+i));
  }
}
function setAllSend(el) {
  setAll(el);
  sendPwm();
}

// ── POWER / TRYB / FADE ──
function setPower(v) {
  fetch(BASE+'/set-power?v='+(v?'1':'0')).then(function(){
    document.getElementById('bdot').style.background = v ? 'var(--cyan)' : 'var(--red)';
    setTimeout(loadStatus, 300);
  });
}
function setTryb(v, btn) {
  document.getElementById('btn-auto').classList.toggle('active', v===1);
  document.getElementById('btn-man').classList.toggle('active', v===0);
  fetch(BASE+'/set-tryb?v='+v).then(function(){ setTimeout(loadStatus, 400); }).catch(function(){});
}
function setFade() {
  var v = document.getElementById('inp-fade').value;
  fetch(BASE+'/set-fade?f='+v).then(function(){ toast('Rampa: '+v+' min \u2013 zapisano'); setTimeout(loadStatus, 500); }).catch(function(){});
}
function setEmaFilter() {
  var v = parseFloat(document.getElementById('inp-ema').value);
  if (isNaN(v) || v < 0.05 || v > 0.50) { toast('Zakres: 0.05 \u2013 0.50', 'err'); return; }
  fetch(BASE+'/set-ema-filter?v='+v.toFixed(2)).then(function(){ toast('Filtr EMA: '+v.toFixed(2)+' \u2013 zapisano'); setTimeout(loadStatus, 500); }).catch(function(){});
}
function setSensInt() {
  var v = parseInt(document.getElementById('inp-sens').value);
  if (isNaN(v) || v < 5 || v > 120) { toast('Zakres: 5 \u2013 120s', 'err'); return; }
  fetch(BASE+'/set-sens-int?v='+v).then(function(){ toast('SENS_INT: '+v+'s \u2013 zapisano'); setTimeout(loadStatus, 500); }).catch(function(){});
}
function setRampSec() {
  var v = parseInt(document.getElementById('inp-ramp').value);
  if (isNaN(v) || v < 5 || v > 120) { toast('Zakres: 5 \u2013 120s', 'err'); return; }
  fetch(BASE+'/set-ramp-sec?v='+v).then(function(){ toast('RAMP_SEC: '+v+'s \u2013 zapisano'); setTimeout(loadStatus, 500); }).catch(function(){});
}

// ── ZAPISZ DO AUTO (EEPROM) ──
function saveToAuto() {
  var pwms = [];
  for (var i = 0; i < 5; i++) {
    var el = document.getElementById('sl'+i);
    pwms.push(el ? el.value : 0);
  }
  var p = pwms.map(function(v,i){ return 'pwm'+i+'='+v; }).join('&');
  fetch(BASE+'/api/save-auto?'+p)
    .then(function(r){ return r.text(); })
    .then(function(m){ toast(m || 'Zapisano do AUTO!'); })
    .catch(function(){ toast('Blad zapisu', 'err'); });
}

// ── SENSOR / LUX TARGET ──
var _sensor = 0, _luxTarget = 2000;
var _pumpOn = false, _camOn = false, _led100On = false;
function setSensor(n) {
  _sensor = n;
  for(var i=0;i<3;i++) document.getElementById('sa-s'+i).classList.toggle('active', i===n);
}
function setLuxSlider(v) {
  _luxTarget = parseInt(v);
  document.getElementById('lux-target-val').textContent = v;
}
function _applyDeviceStyle(btn, lbl, on, label_on, label_off, colorOn) {
  lbl.textContent = on ? label_on : label_off;
  btn.style.borderColor = on ? colorOn : '';
  btn.style.background = on ? 'rgba(0,212,245,.1)' : '';
  btn.style.color = on ? colorOn : '';
}
function toggleDevice(dev) {
  var btn, lbl, url;
  if (dev === 'pump') {
    _pumpOn = !_pumpOn;
    btn = document.getElementById('btn-pump');
    lbl = document.getElementById('lbl-pump');
    url = _pumpOn ? '/api/quick/pump_on' : '/api/quick/pump_off';
    fetch(BASE+url,{method:'POST'}).then(function(){
      _applyDeviceStyle(btn,lbl,_pumpOn,'Pompa WŁ','Pompa WYŁ','var(--cyan)');
    }).catch(function(e){_pumpOn=!_pumpOn;toast('Błąd: '+e,'err');});
  } else if (dev === 'camera') {
    _camOn = !_camOn;
    btn = document.getElementById('btn-camera');
    lbl = document.getElementById('lbl-camera');
    url = _camOn ? '/api/quick/camera_on' : '/api/quick/camera_off';
    fetch(BASE+url,{method:'POST'}).then(function(){
      _applyDeviceStyle(btn,lbl,_camOn,'Kamera WŁ','Kamera WYŁ','var(--teal)');
    }).catch(function(e){_camOn=!_camOn;toast('Błąd: '+e,'err');});
  }
}
function toggleLed100() {
  _led100On = !_led100On;
  var btn=document.getElementById('btn-led100'), lbl=document.getElementById('lbl-led100');
  var url = _led100On ? '/api/quick/led_test' : '/api/quick/led_off';
  fetch(BASE+url,{method:'POST'}).then(function(){
    _applyDeviceStyle(btn,lbl,_led100On,'LED 100% WŁ','LED 100%','var(--warm)');
  }).catch(function(e){_led100On=!_led100On;toast('Błąd: '+e,'err');});
}

// ── ADAPTIVE SAVE ──
function saveAdaptive() {
  var ad = document.getElementById('tog-adapt').checked ? 1 : 0;
  var ln = document.getElementById('tog-learn').checked ? 1 : 0;
  var ml = document.getElementById('tog-minlux').checked ? 1 : 0;
  var li = parseInt(document.getElementById('minlux-interval').value) || 30;
  fetch(BASE+'/api/adaptive/save', {method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({sensorMode:_sensor,adaptEnabled:ad,learningEnabled:ln})})
    .then(function(){
      fetch(BASE+'/api/minlux/save', {method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({enabled:ml,target:_luxTarget,interval:li})})
        .then(function(){ toast('Zapisano regulacje adaptacyjna'); setTimeout(loadStatus, 500); });
    }).catch(function(e){toast('Bl\u0105d: '+e,'err');});
}

// ── CUSTOM TIME PICKER ──
function buildTimeSel(containerId, hiddenId) {
  var c = document.getElementById(containerId);
  if (!c) return;
  var hSel = document.createElement('select');
  var mSel = document.createElement('select');
  var sep  = document.createElement('span');
  sep.className = 'time-sel-sep'; sep.textContent = ':';
  for (var h = 0; h < 24; h++) {
    var o = document.createElement('option');
    o.value = h; o.text = String(h).padStart(2,'0');
    hSel.appendChild(o);
  }
  for (var m = 0; m < 60; m++) {
    var o2 = document.createElement('option');
    o2.value = m; o2.text = String(m).padStart(2,'0');
    mSel.appendChild(o2);
  }
  c.appendChild(hSel); c.appendChild(sep); c.appendChild(mSel);
  c._hSel = hSel; c._mSel = mSel;
}
function setTimeSel(containerId, hhmm) {
  var c = document.getElementById(containerId); if(!c||!c._hSel) return;
  var parts = (hhmm||'00:00').split(':');
  c._hSel.value = parseInt(parts[0]||0,10);
  c._mSel.value = parseInt(parts[1]||0,10);
}
function getTimeSel(containerId) {
  var c = document.getElementById(containerId); if(!c||!c._hSel) return '00:00';
  return String(c._hSel.value).padStart(2,'0')+':'+String(c._mSel.value).padStart(2,'0');
}
(function initTimeSelectors(){
  ['ts-mw','ts-me','ts-mo','ts-eo'].forEach(function(id){ buildTimeSel(id); });
})();

// ── EB DISPLAY ──
function updateEbDisplay() {
  var eb = document.getElementById('s-eb');
  if (!eb) return;
  var offset = parseInt(eb.value, 10) || 0;
  var sunsetMin = window._sunsetMin || 0;
  var resultMin = ((sunsetMin + offset) + 1440) % 1440;
  var lbl = document.getElementById('eb-offset-label');
  if (lbl) lbl.textContent = (offset <= 0 ? offset : '+' + offset) + ' min';
  var res = document.getElementById('eb-result-time');
  if (res) res.textContent = String(Math.floor(resultMin/60)).padStart(2,'0') + ':' + String(resultMin%60).padStart(2,'0');
}

// ── SCHEDULE ──
function saveSched() {
  var p='mw='+encodeURIComponent(getTimeSel('ts-mw'))
    +'&me='+encodeURIComponent(getTimeSel('ts-me'))
    +'&mo='+encodeURIComponent(getTimeSel('ts-mo'))
    +'&eb='+encodeURIComponent(document.getElementById('s-eb').value)
    +'&eo='+encodeURIComponent(getTimeSel('ts-eo'));
  fetch(BASE+'/set-schedule?'+p).then(function(){ toast('Harmonogram zapisany'); setTimeout(loadStatus, 500); }).catch(function(e){toast('Bl\u0105d: '+e,'err');});
}

// ── PUMP SCHEDULE ──
var _pumpSlots = [];
function renderPumpSlots() {
  var c = document.getElementById('pump-slots'); c.innerHTML = '';
  _pumpSlots.forEach(function(s, i) {
    var d = document.createElement('div'); d.className = 'pump-slot-row';
    d.innerHTML = '<div class="time-field" style="flex:1"><label>Przedzial '+(i+1)+' &#8211; Start</label><div class="time-sel" id="ps'+i+'"></div></div>'
      +'<div class="time-field" style="flex:1"><label>Koniec</label><div class="time-sel" id="pe'+i+'"></div></div>'
      +'<button class="pump-rm" onclick="rmPumpSlot('+i+')">&#10005;</button>';
    c.appendChild(d);
    buildTimeSel('ps'+i); setTimeSel('ps'+i, s.s);
    buildTimeSel('pe'+i); setTimeSel('pe'+i, s.e);
  });
}
function addPumpSlot() {
  if (_pumpSlots.length >= 6) { toast('Maks. 6'); return; }
  _pumpSlots.push({s:'00:00',e:'00:00'}); renderPumpSlots();
}
function rmPumpSlot(i) { _pumpSlots.splice(i,1); renderPumpSlots(); }
function savePump() {
  var slots = [];
  _pumpSlots.forEach(function(_, i) {
    var sv = getTimeSel('ps'+i), ev = getTimeSel('pe'+i);
    if (sv&&ev) slots.push({s:sv,e:ev});
  });
  if (!slots.length) { toast('Dodaj przedzial'); return; }
  fetch(BASE+'/set-pump-schedule', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(slots)})
    .then(function(r){return r.text();}).then(function(m){ toast('Pompka zapisana ('+slots.length+')'); setTimeout(loadStatus, 500); }).catch(function(e){toast('Bl\u0105d: '+e,'err');});
}

// ── QUICK POST ──
function qPost(url) {
  fetch(BASE+url, {method:'POST'}).then(function(r){return r.text();}).then(function(m){if(m&&m.length<200)toast(m);}).catch(function(e){toast('Błąd: '+e,'err');});
}

// ── LOAD STATUS ──
function hhmm(m){return String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0');}
function gid(id){return document.getElementById(id);}
function loadStatus() {
  fetch(BASE+'/api/status').then(function(r){return r.text();}).then(function(t){
    var d;
    try { d = JSON.parse(t); } catch(e) { return; }
    // Power
    var tp=gid('tog-power'); if(tp) tp.checked=!!d.power;
    var bd=gid('bdot'); if(bd) bd.style.background=d.power?'var(--cyan)':'var(--red)';
    // Mode
    var ba=gid('btn-auto'); if(ba) ba.classList.toggle('active',!!d.tryb);
    var bm=gid('btn-man');  if(bm) bm.classList.toggle('active',!d.tryb);
    // Fade
    var fi=gid('inp-fade'); if(fi&&d.fadeMinutes) fi.value=d.fadeMinutes;
    var fe=gid('inp-ema');  if(fe&&d.emaFilter)   fe.value=parseFloat(d.emaFilter).toFixed(2);
    var fs=gid('inp-sens'); if(fs&&d.sensInt)     fs.value=d.sensInt;
    var fr=gid('inp-ramp'); if(fr&&d.rampSec)     fr.value=d.rampSec;
    // PWM
    if (d.pwm && Array.isArray(d.pwm)) {
      var avg=0;
      for(var i=0;i<5;i++){
        var v=d.pwm[i]||0;
        var sl=gid('sl'+i); if(sl){sl.value=v;updFill(sl);}
        setValLabel('val-'+i,v);
        avg+=v;
      }
      avg=Math.round(avg/5);
      var sa=gid('sl-all'); if(sa){sa.value=avg;updFill(sa);}
      setValLabel('val-all',avg);
    }
    // Temps
    if (d.temps && Array.isArray(d.temps)) {
      var t1=gid('d-t1'); if(t1) t1.innerHTML=(d.temps[0]!=null?parseFloat(d.temps[0]).toFixed(1):'--')+'<span class="unit">&#176;C</span>';
      var t2=gid('d-t2'); if(t2) t2.innerHTML=(d.temps[1]!=null?parseFloat(d.temps[1]).toFixed(1):'--')+'<span class="unit">&#176;C</span>';
      var tw=gid('d-tw'); if(tw) tw.innerHTML=(d.temps[2]!=null?parseFloat(d.temps[2]).toFixed(1):'--')+'<span class="unit">&#176;C</span>';
    }
    // Lux - elementy moga nie istniec na tej stronie
    if (d.luxRoom !== undefined) {
      var lv2=gid('lux-val'); if(lv2) lv2.textContent=Math.round(d.luxRoom)+' lx';
      var lb2=gid('lux-bar'); if(lb2) lb2.style.width=Math.min(100,d.luxRoom/50)+'%';
    }
    // Sensor mode
    if (d.sensorMode !== undefined) setSensor(d.sensorMode);
    // Adaptive
    var ta=gid('tog-adapt'); if(ta&&d.adaptEnabled!==undefined) ta.checked=!!d.adaptEnabled;
    var tl=gid('tog-learn'); if(tl&&d.learningEnabled!==undefined) tl.checked=!!d.learningEnabled;
    var tm=gid('tog-minlux'); if(tm&&d.minLuxEnabled!==undefined) tm.checked=!!d.minLuxEnabled;
    if (d.minLuxTarget) {
      var lt=parseInt(d.minLuxTarget); _luxTarget=lt;
      var sll=gid('sl-lux'); if(sll){sll.value=lt;updFill(sll);}
      var lvv=gid('lux-target-val'); if(lvv) lvv.textContent=lt;
    }
    var mli=gid('minlux-interval'); if(mli&&d.minLuxInterval) mli.value=parseInt(d.minLuxInterval);
    // Harmonogram
    if (d.schedule) {
      var sc=d.schedule;
      if(sc.morningWD!==undefined) setTimeSel('ts-mw',hhmm(sc.morningWD));
      if(sc.morningWE!==undefined) setTimeSel('ts-me',hhmm(sc.morningWE));
      if(sc.middayOff!==undefined) setTimeSel('ts-mo',hhmm(sc.middayOff));
      var eb=gid('s-eb'); if(eb&&sc.eveningBefore!==undefined) { eb.value=sc.eveningBefore; }
      if(sc.eveningOff!==undefined) setTimeSel('ts-eo',hhmm(sc.eveningOff));
      if(sc.sunsetMin!==undefined) {
        window._sunsetMin = sc.sunsetMin;
        var st=gid('eb-sunset-time');
        if(st) st.textContent=hhmm(sc.sunsetMin);
      }
      updateEbDisplay();
    }
    // Pompka
    if (d.pumpSlots && Array.isArray(d.pumpSlots)) {
      _pumpSlots=d.pumpSlots.map(function(sl){return {s:hhmm(sl.start||0),e:hhmm(sl.end||0)};});
      renderPumpSlots();
    }
    // [SIM-UI] Zaktualizuj kartę symulacji czujnika (usuń razem z kartą HTML i funkcjami simLoad/simAction)
    simLoad(d);
  }).catch(function(){});
}

// ── LOAD LOGS ──
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM-UI] Funkcje JS do sterowania symulacją czujnika.
// Aby usunąć: usuń ten blok (od [SIM-UI] do [SIM-UI] koniec JS)
// i wywołanie simLoad() w loadStatus().
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
function simLoad(d) {
  // d = obiekt z /api/status
  var badge = document.getElementById('sim-badge');
  var live  = document.getElementById('sim-lux-live');
  if (!badge) return;
  if (d.simEnabled) {
    var mode = d.simAuto ? 'AUTO-SIN' : 'STAŁY';
    badge.textContent = '🎭 SYMULACJA ' + mode;
    badge.style.background = d.simAuto ? 'rgba(255,179,71,.2)' : 'rgba(0,212,245,.2)';
    badge.style.color = d.simAuto ? '#ffb347' : 'var(--cyan)';
    badge.style.border = d.simAuto ? '1px solid rgba(255,179,71,.4)' : '1px solid rgba(0,212,245,.4)';
    live.textContent = d.simSmoothed + ' lux (wygładzone)';
  } else {
    badge.textContent = '⚙️ WYŁĄCZONA - hardware';
    badge.style.background = 'rgba(255,255,255,.06)';
    badge.style.color = 'var(--text-dim)';
    badge.style.border = '1px solid var(--border)';
    live.textContent = '';
  }
  // Uzupełnij pola wartościami z ESP
  var sv = document.getElementById('sim-val-slider');
  var sn = document.getElementById('sim-val-num');
  if (sv && !d.simAuto) { sv.value = Math.min(d.simValue, 10000); sn.value = d.simValue; }
  var mn = document.getElementById('sim-auto-min');
  var mx = document.getElementById('sim-auto-max');
  var pr = document.getElementById('sim-auto-period');
  if (mn) mn.value = d.simMin;
  if (mx) mx.value = d.simMax;
  if (pr) pr.value = d.simPeriodS;
}

function simAction(action) {
  fetch(BASE+'/api/sim?action='+encodeURIComponent(action), {method:'GET'})
  .then(function(r){ if(r.ok) { toast('🎭 Symulacja: ' + action, 'ok'); setTimeout(loadStatus, 400); }
                     else toast('Błąd symulacji', 'err'); })
  .catch(function(){ toast('Błąd połączenia', 'err'); });
}

function simSetValue() {
  var val = document.getElementById('sim-val-num').value;
  fetch(BASE+'/api/sim?action=set&value='+encodeURIComponent(val), {method:'GET'})
  .then(function(r){ if(r.ok) { toast('🎭 Lux ustawiony: ' + val, 'ok'); setTimeout(loadStatus, 400); }
                     else toast('Błąd symulacji', 'err'); })
  .catch(function(){ toast('Błąd połączenia', 'err'); });
}

function simSetAuto() {
  var mn  = document.getElementById('sim-auto-min').value;
  var mx  = document.getElementById('sim-auto-max').value;
  var per = document.getElementById('sim-auto-period').value;
  fetch(BASE+'/api/sim?action=set&min='+encodeURIComponent(mn)+'&max='+encodeURIComponent(mx)+'&period='+encodeURIComponent(per), {method:'GET'})
  .then(function(r){ if(r.ok) { toast('🎭 Parametry AUTO zapisane', 'ok'); setTimeout(loadStatus, 400); }
                     else toast('Błąd symulacji', 'err'); })
  .catch(function(){ toast('Błąd połączenia', 'err'); });
}
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
// [SIM-UI] koniec funkcji JS symulacji
// ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

function loadLogStats() { loadFsList(); }

var _fsList = [];
var _fsCurrentFile = '';

function loadFsList() {
  var FS_LIMIT = 10420224;
  fetch(BASE+'/api/fs-list').then(function(r){return r.json();}).then(function(d){
    _fsList = d.files || [];

    // ── pasek dysku ──
    var used = d.used || 0;
    var pct  = Math.round(used * 100 / FS_LIMIT);
    var kb   = (used / 1024).toFixed(0);
    var hue  = Math.round(160 - Math.min(pct, 100) * 1.6);   // 160=turkus(0%) -> 0=czerwień(100%)
    var hue2 = Math.max(0, hue - 25);
    var barColor  = 'linear-gradient(90deg,hsl('+hue+',90%,48%),hsl('+hue2+',95%,55%))';
    var pctColor  = 'hsl('+hue+',85%,62%)';
    var eu = document.getElementById('log-used');   if (eu) { eu.textContent = kb + ' KB'; eu.style.color = pctColor; }
    var ep = document.getElementById('log-pct');    if (ep) { ep.textContent = '(' + pct + '%)'; ep.style.color = pctColor; }
    var eb = document.getElementById('log-bar');    if (eb) { eb.style.width = Math.min(pct,100)+'%'; eb.style.background = barColor; }
    var et = document.getElementById('log-total');  if (et) { var tot = d.total || FS_LIMIT; et.textContent = ' / ' + (tot >= 1048576 ? (tot/1048576).toFixed(0)+' MB' : (tot/1024).toFixed(0)+' KB'); }

    // ── tabela plików ──
    var fll = document.getElementById('fs-file-list');
    if (!fll) return;
    if (!_fsList.length) {
      fll.innerHTML = '<div style="color:var(--text-dim);font-size:.78rem;padding:4px 0">Brak plik&#243;w na LittleFS</div>';
      return;
    }
    var ext2icon = {'.txt':'📄','.log':'📋','.csv':'📊','.json':'📦','.bin':'⚙️'};
    var rows = '<div style="display:flex;flex-direction:column;gap:4px">';
    _fsList.forEach(function(f){
      var fp2 = Math.round(f.size*100/FS_LIMIT);
      var pc = fp2 > 50 ? '#ff4d6d' : fp2 > 25 ? '#ffb347' : 'var(--cyan)';
      var ext = f.name.match(/\.[^.]+$/); ext = ext ? ext[0].toLowerCase() : '';
      var icon = ext2icon[ext] || '📄';
      var shortName = f.name.replace(/^\//,'');
      rows += '<div style="display:flex;align-items:center;gap:8px;padding:6px 10px;background:rgba(255,255,255,.03);border:1px solid rgba(255,255,255,.06);border-radius:8px;transition:background .15s" onmouseover="this.style.background=\'rgba(0,212,245,.06)\'" onmouseout="this.style.background=\'rgba(255,255,255,.03)\'">' +
        '<span style="font-size:.95rem">' + icon + '</span>' +
        '<code style="flex:1;font-size:.75rem;color:var(--cyan);overflow:hidden;text-overflow:ellipsis;white-space:nowrap" title="' + f.name + '">' + shortName + '</code>' +
        '<span style="font-size:.72rem;font-family:Space Mono,monospace;color:' + pc + ';min-width:52px;text-align:right">' + fmtBytes(f.size) + '</span>' +
        '<div style="display:flex;gap:4px">' +
          '<button onclick="fsViewFile(\'' + f.name + '\')" title="Podejrzyj" style="padding:3px 8px;background:rgba(0,212,245,.1);border:1px solid rgba(0,212,245,.2);color:#00d4f5;border-radius:6px;cursor:pointer;font-size:.75rem">&#128065;</button>' +
          '<button onclick="fsDownloadFile(\'' + f.name + '\')" title="Pobierz" style="padding:3px 8px;background:rgba(122,212,255,.1);border:1px solid rgba(122,212,255,.2);color:#7ad4ff;border-radius:6px;cursor:pointer;font-size:.75rem">&#128190;</button>' +
          '<button onclick="fsDeleteFile(\'' + f.name + '\')" title="Usu&#324;" style="padding:3px 8px;background:rgba(255,77,109,.1);border:1px solid rgba(255,77,109,.25);color:#ff4d6d;border-radius:6px;cursor:pointer;font-size:.75rem">&#128465;</button>' +
        '</div></div>';
    });
    rows += '</div>';
    fll.innerHTML = rows;
  }).catch(function(){
    var fll = document.getElementById('fs-file-list');
    if (fll) fll.innerHTML = '<div style="color:#ff4d6d;font-size:.78rem;padding:4px 0">&#10060; B&#322;&#261;d &#322;adowania listy plik&#243;w (sprawdź czy LittleFS jest gotowy)</div>';
  });
}

function fmtBytes(b) {
  if (b < 1024) return b + ' B';
  if (b < 1048576) return (b/1024).toFixed(1) + ' KB';
  return (b/1048576).toFixed(2) + ' MB';
}

function pctColor(p) {
  return { pct: p, col: p > 90 ? '#ff4d6d' : p > 70 ? '#ffb347' : 'var(--cyan)' };
}

function fsView() {
  var fname = (document.getElementById('fs-sel')||{}).value;
  if (!fname) { toast('Wybierz plik', 'err'); return; }
  fsViewFile(fname);
}

function fsViewFile(fname) {
  _fsCurrentFile = fname;
  var lines = parseInt(((document.getElementById('fs-lines')||{}).value) || '100');
  var viewer = document.getElementById('fs-viewer');
  var pre    = document.getElementById('fs-content');
  var title  = document.getElementById('fs-viewer-title');
  if (!pre || !viewer) return;
  viewer.style.display = 'block';
  pre.textContent = '⏳ Ładowanie ' + fname + '...';
  if (title) title.textContent = fname.replace(/^\//,'') + (lines > 0 ? ' · ostatnie ' + lines + ' linii' : ' · całość');
  var url = BASE + '/api/fs-view?file=' + encodeURIComponent(fname) + (lines > 0 ? '&lines='+lines : '');
  fetch(url).then(function(r){ return r.text(); }).then(function(t){
    pre.textContent = t || '(plik pusty)';
    pre.scrollTop = pre.scrollHeight;
  }).catch(function(){ pre.textContent = '[ERR] Błąd ładowania pliku'; });
  // przewiń do podglądu
  setTimeout(function(){ viewer.scrollIntoView({behavior:'smooth',block:'nearest'}); }, 150);
}

function fsReload() {
  if (_fsCurrentFile) fsViewFile(_fsCurrentFile);
}

function fsDownload() {
  var fname = (document.getElementById('fs-sel')||{}).value;
  if (!fname) { toast('Wybierz plik', 'err'); return; }
  fsDownloadFile(fname);
}

function fsDownloadFile(fname) {
  window.location.href = BASE + '/api/fs-download?file=' + encodeURIComponent(fname);
}

function fsDelete() {
  var fname = (document.getElementById('fs-sel')||{}).value;
  if (!fname) { toast('Wybierz plik', 'err'); return; }
  fsDeleteFile(fname);
}

function fsDeleteFile(fname) {
  if (!confirm('Usunąć plik ' + fname + '?')) return;
  fetch(BASE + '/api/fs-delete?file=' + encodeURIComponent(fname), {method:'POST'})
    .then(function(r){ return r.text(); })
    .then(function(){
      toast('Usunięto: ' + fname, 'ok');
      if (_fsCurrentFile === fname) {
        _fsCurrentFile = '';
        var v = document.getElementById('fs-viewer'); if (v) v.style.display = 'none';
      }
      loadFsList();
    }).catch(function(){ toast('Błąd usuwania', 'err'); });
}

function loadLogs(lines) {
  var n = lines !== undefined ? lines : 100;
  var el = document.getElementById('logs-content');
  if (!el) return;
  el.textContent = '⏳ Ładowanie logów...';
  el.style.color = '#69f0ae';
  fetch(BASE+'/api/log-lines?lines='+n)
    .then(function(r){ return r.text(); })
    .then(function(t){
      _logsRaw = t || '';
      lRender();
      el.scrollTop = el.scrollHeight;
      loadLogStats();
    })
    .catch(function(e){ el.textContent = '[ERR] Błąd: ' + e; el.style.color = '#ff4d6d'; });
}

// ── FILTRY ZAKŁADKI LOGI ──
var _logsRaw = '';
var _lCat = 'all';

function lClassify(line) {
  var t = line.toLowerCase();
  // BŁĘDY — czerwony
  if (/[ERR]|🚨|awaria|brak czasu ntp|brak wifi|littlefs błąd|crash|critical|heap krytyczny/.test(t)) return 'err';
  // OSTRZEŻENIA — żółty  
  if (/[WARN]️|warn|saturacja ir|czujnik zwrócił 0|brak połączenia|cicha zmiana|diag-/.test(t)) return 'warn';
  // TEMPERATURY — pomarańczowy
  if (/ds18b20|temp płyta|płyta1|płyta2|strefa1|strefa2|derating|redukcja mocy|przegrzanie|°c|woda.*błąd/.test(t)) return 'temp';
  // RAMPY — niebieski
  if (/miękki start|przejście auto|przejście manualne|przejście manual->auto|przejście auto->manual|rampa start|rampa koniec|mid-ramp|dobieg|ramp.*init|fade|soft.start|transition|wznosz|opadan/.test(t)) return 'ramp';
  // POMPA — fioletowy
  if (/pompa|pump|💦/.test(t)) return 'pump';
  // ADAPTACJA — zielony
  if (/adapt|🌿|min lux|tsl|lux |transmis|uczeni|korekta min|korekta max|redukcja led|⚖️|🔆|🧠|próbek/.test(t)) return 'adapt';
  // NTP / SIEĆ — szary
  if (/ntp|wifi|cloud|mqtt|połączono|synchron|zachód słońca|sunset|strefa czas|iot|chmura|ip urządz/.test(t)) return 'ntp';
  // LED/PWM — biały
  if (/pwm|led|jasność|brightness|flash|backupbright|diod|kanał|zasilanie|wybór.*led/.test(t)) return 'led';
  return 'all';
}

function lSetCat(cat, el) {
  _lCat = cat;
  document.querySelectorAll('#logi-chips .t-chip').forEach(function(c){ c.classList.remove('on'); });
  el.classList.add('on');
  lRender();
}

function lApplyFilter() { lRender(); }

function lRender() {
  var el = document.getElementById('logs-content');
  if (!el || !_logsRaw) return;
  var search = (document.getElementById('logi-search') || {}).value || '';
  var lines = _logsRaw.split('\n');
  var out = [];
  lines.forEach(function(line) {
    if (!line.trim()) return;
    var cat = lClassify(line);
    if (_lCat !== 'all' && cat !== _lCat) return;
    if (search && line.toLowerCase().indexOf(search.toLowerCase()) < 0) return;
    out.push(line);
  });
  el.textContent = out.length ? out.join('\n') : '(brak wyników dla wybranego filtra)';
  el.scrollTop = el.scrollHeight;
}

// ── DASHBOARD ──
function loadDash() {
  var n=new Date();
  var dt=document.getElementById('dash-time');
  if(dt) dt.textContent=String(n.getHours()).padStart(2,'0')+':'+String(n.getMinutes()).padStart(2,'0')+':'+String(n.getSeconds()).padStart(2,'0');
  fetch(BASE+'/api/status').then(function(r){return r.json();}).then(function(d){
    var efw=document.getElementById('dash-fw'); if(efw && d.fwVersion) efw.textContent=d.fwVersion;
    function fmtT(v){return (v!==undefined&&v!==null&&!isNaN(v)&&v!==0)?parseFloat(v).toFixed(1)+'<span class="unit">&#176;C</span>':'&#8212;<span class="unit">&#176;C</span>';}
    var t0=d.temps&&d.temps[0], t1=d.temps&&d.temps[1], t2=d.temps&&d.temps[2];
    var e=document.getElementById('d-t1'); if(e) e.innerHTML=fmtT(t0);
    e=document.getElementById('d-t2'); if(e) e.innerHTML=fmtT(t1);
    e=document.getElementById('d-tw'); if(e) e.innerHTML=fmtT(t2);
    var lr=d.luxRoom||0;
    e=document.getElementById('d-lux-room'); if(e) e.textContent=Math.round(lr);
    e=document.getElementById('d-lux-water'); if(e) e.textContent=Math.round(d.luxNadWoda||d.luxWater||0);
    e=document.getElementById('d-lux-bar'); if(e) e.style.width=Math.min(100,lr/50)+'%';
    if(d.pwm){
      var sumPWM=0;
      for(var i=0;i<5;i++){var pct=Math.round((d.pwm[i]||0)/1023*100);var b=document.getElementById('d-pb'+i);var v=document.getElementById('d-pv'+i);if(b)b.style.width=pct+'%';if(v)v.textContent=(d.pwm[i]||0)+' ('+pct+'%)';sumPWM+=(d.pwm[i]||0);}
      var avgPWM=Math.round(sumPWM/5), avgPct=Math.round(avgPWM/1023*100);
      var luxPPwm  = parseFloat(d.luxPerPwm)  || 13.02;
      var glassT   = parseFloat(d.glassTransm) || 0.85;
      var minLuxT  = (d.minLuxEnabled && d.minLuxTarget) ? parseInt(d.minLuxTarget) : 0;
      var sunRoom  = Math.max(0, parseFloat(d.luxRoom) || 0);
      var sunAtPlants  = Math.round(sunRoom * glassT);
      // LED tile — ZAWSZE z realnego PWM (co LED faktycznie emituje)
      var ledAtPlants  = Math.round(avgPWM * luxPPwm);  // LED nad wodą – bez korekcji szyby bocznej
      var totalAtPlants = sunAtPlants + ledAtPlants;
      // Słońce tile
      var sp=document.getElementById('d-sun-pct'); if(sp) sp.textContent='~'+sunAtPlants;
      var sl2=document.getElementById('d-sun-lux'); if(sl2) sl2.textContent=Math.round(sunRoom)+' lx pokój';
      // LED tile
      var lv=document.getElementById('d-led-val');
      if(lv) lv.textContent='~'+ledAtPlants;
      var lrw=document.getElementById('d-led-pwm-raw');
      if(lrw) lrw.textContent=avgPWM+' PWM ('+avgPct+'%)';
      var lp=document.getElementById('d-led-pct'); if(lp) lp.textContent='';
      // Suma pod barem
      var tot=document.getElementById('d-lux-total');
      if(tot){
        // [OK] FIX-v22: mianownik = ta sama skala co pasek
        var inWindowTot = d.inLightingWindow === true || d.inLightingWindow === 'true';
        var autoLuxTot  = Math.round((d.autoPWM || 0) * luxPPwm * glassT);
        var scaleTot = (inWindowTot && autoLuxTot > 0) ? autoLuxTot : (minLuxT || totalAtPlants || 500);
        var pct = Math.round(totalAtPlants / scaleTot * 100);
        var col = pct >= 95 ? '#4ade80' : pct >= 70 ? '#ffd32a' : '#ff6b6b';
        tot.innerHTML = '~'+totalAtPlants+' / '+scaleTot+' lx <span style="color:'+col+'"">('+pct+'%)</span>';
      }

      // [OK] FIX-v22-BARSCALE: skala dynamiczna
      // W oknie świecenia: skala = lux suwaka | Poza: skala = cel MIN LUX
      var inWindow = d.inLightingWindow === true || d.inLightingWindow === 'true';
      var autoLuxScale = Math.round((d.autoPWM || 0) * luxPPwm);  // j.w. – skala bez korekcji szyby
      var barScale;
      if (inWindow && autoLuxScale > 0) { barScale = autoLuxScale; }
      else if (minLuxT > 0) { barScale = minLuxT; }
      else { barScale = Math.max(totalAtPlants, 500); }
      var scLabel = document.getElementById('d-bar-scale');
      if(scLabel) scLabel.textContent = 'skala 0-' + barScale.toLocaleString() + ' lx (przy rośblinach)';
      // Bar: używamy wartości PRZY ROŚLINACH
      var sunPct = Math.min(100, sunAtPlants / barScale * 100);
      var ledPct = Math.min(100, ledAtPlants  / barScale * 100);
      var total = sunPct + ledPct; var scale2 = total > 100 ? 100/total : 1;
      (function(){
        var canvas = document.getElementById('d-blend-bar');
        if (!canvas) return;
        // Polyfill roundRect dla starszych przeglądarek mobilnych
        if (!CanvasRenderingContext2D.prototype.roundRect) {
          CanvasRenderingContext2D.prototype.roundRect = function(x,y,w,h,r){
            this.beginPath();
            this.moveTo(x+r,y);this.lineTo(x+w-r,y);this.quadraticCurveTo(x+w,y,x+w,y+r);
            this.lineTo(x+w,y+h-r);this.quadraticCurveTo(x+w,y+h,x+w-r,y+h);
            this.lineTo(x+r,y+h);this.quadraticCurveTo(x,y+h,x,y+h-r);
            this.lineTo(x,y+r);this.quadraticCurveTo(x,y,x+r,y);this.closePath();return this;
          };
        }
        var W = canvas.offsetWidth || canvas.parentElement.offsetWidth || 300;
        canvas.width = W; canvas.height = 14;
        var ctx = canvas.getContext('2d');
        ctx.clearRect(0,0,W,14);
        // tło
        ctx.fillStyle = 'rgba(255,255,255,0.06)';
        ctx.roundRect(0,0,W,14,7); ctx.fill();
        // wypełnienie proporcjonalne
        var totalFillPct = (sunPct + ledPct) * scale2;
        var fillW = Math.min(totalFillPct / 100 * W, W);
        if (fillW > 2) {
          var sunFrac = (sunPct * scale2) / Math.max(totalFillPct, 0.01);
          sunFrac = Math.min(sunFrac, 1);
          var grd = ctx.createLinearGradient(0,0,fillW,0);
          // Gradient płynny: pomarańcz (słońce) -> cyan (LED)
          grd.addColorStop(0, '#ff8800');
          grd.addColorStop(Math.max(0, sunFrac - 0.08), '#ffcc00');
          if (sunFrac < 1) {
            grd.addColorStop(Math.min(sunFrac + 0.08, 1), '#0099ff');
            grd.addColorStop(1, '#00d4f5');
          }
          ctx.fillStyle = grd;
          ctx.roundRect(0,0,fillW,14,7); ctx.fill();
        }
        var pctEl = document.getElementById('d-bar-pct');
        if (pctEl) pctEl.textContent = '💡' + Math.round(ledPct*scale2) + '%';
        var sunPctEl = document.getElementById('d-bar-sun-pct');
        if (sunPctEl) {
          var sp = Math.round(sunPct * scale2);
          sunPctEl.textContent = sp > 0 ? '☀️' + sp + '%' : '';
        }
      })();
      var sp=document.getElementById('d-sun-pct'); if(sp) sp.textContent='~'+sunAtPlants;
      var sl2=document.getElementById('d-sun-lux'); if(sl2) sl2.textContent=Math.round(sunRoom)+' lx pokój';
      e=document.getElementById('d-lux-bar'); if(e) e.style.width=Math.min(100, sunAtPlants/barScale*100).toFixed(1)+'%';
    }
    // ── Watty per kanał + Power Balancer ──
    var totW = parseFloat(d.powerNowW)||0;
    var limW = parseFloat(d.powerLimitW)||90;
    var chW  = d.powerNowCh || [0,0,0,0,0];
    for(var i=0;i<5;i++){var pw=document.getElementById('d-pw'+i);if(pw)pw.textContent=parseFloat(chW[i]||0).toFixed(1)+'W';}
    var pbl=document.getElementById('d-power-bar-label');
    if(pbl)pbl.textContent=totW.toFixed(1)+' W / '+limW.toFixed(0)+' W';
    var pbf=document.getElementById('d-power-bar-fill');
    if(pbf)pbf.style.width=Math.min(100,totW/limW*100).toFixed(1)+'%';
    var pbp=document.getElementById('d-power-bar-pct');
    if(pbp)pbp.textContent=Math.round(totW/limW*100)+'%';
    var pbm=document.getElementById('d-power-bar-max');
    if(pbm)pbm.textContent=limW.toFixed(0)+' W';
    var pnw=document.getElementById('d-power-now-w');
    if(pnw)pnw.textContent=totW.toFixed(1)+' W';
    e=document.getElementById('d-power'); if(e) e.innerHTML=d.power?'<span style="color:var(--cyan)">WŁĄCZONE</span>':'<span style="color:var(--red)">WYŁĄCZONE</span>';
    e=document.getElementById('d-mode'); if(e) e.innerHTML=d.tryb?'<span style="color:var(--cyan)">AUTO</span>':'<span style="color:var(--warm)">MANUAL</span>';
    e=document.getElementById('d-adapt'); if(e) e.innerHTML=d.adaptEnabled?'<span style="color:var(--cyan)">Aktywna</span>':'<span style="color:var(--text-dim)">Wył.</span>';
    var at=document.getElementById('d-adapt-trans'); if(at) at.innerHTML=d.adaptEnabled&&d.adaptTransmisja?'transmisja <span style="color:var(--cyan)">'+(d.adaptTransmisja||0)+'%</span>':'';
    e=document.getElementById('d-pump'); if(e) e.innerHTML=d.pumpOn?'<span style="color:var(--cyan)">WŁ &#128167;</span>':'<span style="color:var(--text-dim)">WYŁ</span>';
    e=document.getElementById('d-minlux'); if(e) e.innerHTML=d.minLuxEnabled?'<span style="color:var(--cyan)">'+(d.minLuxTarget||2000)+' lx</span>':'<span style="color:var(--text-dim)">Wył.</span>';
    var mt=document.getElementById('d-minlux-target');
    var ms=document.getElementById('d-minlux-state');
    if(mt) mt.innerHTML=d.minLuxEnabled?'<span style="color:var(--cyan)">'+(d.minLuxTarget||2000)+' lx</span>':'<span style="color:var(--text-dim)">Wył.</span>';
    if(ms){if(!d.minLuxEnabled){ms.innerHTML='<span style="color:var(--text-dim)">nieaktywny</span>';}else if(d.minLuxActive){ms.innerHTML='<span style="color:#4caf50">&#9679; uzupe&#322;nia teraz</span>';}else{ms.innerHTML='<span style="color:var(--text-dim)">&#9679; wystarczy &#347;wiat&#322;a</span>';}}
    var sn=['OFF','Pokój','Pokój+Woda']; e=document.getElementById('d-sensor'); if(e) e.innerHTML='<span style="color:var(--cyan)">'+(sn[d.sensorMode||0])+'</span>';
    e=document.getElementById('d-fade'); if(e) e.innerHTML='<span style="color:var(--cyan)">'+(d.fadeMinutes||'--')+' min</span>';

    // ── Ramp widget update ──
    (function(){
      var active = !!d.rampActive;
      var inactiveEl = document.getElementById('d-ramp-inactive');
      var activeEl   = document.getElementById('d-ramp-active');
      if(inactiveEl) inactiveEl.style.display = active ? 'none' : '';
      if(activeEl)   activeEl.style.display   = active ? '' : 'none';

      if(!active) return;

      var elapsed = d.rampElapsedMs || 0;
      var total   = d.rampTotalMs   || 1;
      var pct     = Math.min(100, Math.round(elapsed / total * 100));
      var remain  = Math.max(0, total - elapsed);
      var remMin  = Math.floor(remain / 60000);
      var remSec  = Math.floor((remain % 60000) / 1000);
      var timeStr = String(remMin).padStart(2,'0') + ':' + String(remSec).padStart(2,'0');
      var dirLabel = d.rampUp ? '&#9650; wznoszenie' : '&#9660; opadanie';
      var icon     = d.rampUp ? '&#127748;' : '&#127753;';

      // Chip RAMPA (mały kafelek w statusach)
      var arc = document.getElementById('d-ramp-arc');
      if(arc){ var c=75.4; arc.setAttribute('stroke-dashoffset', (c*(1-pct/100)).toFixed(2)); }
      var ptxt = document.getElementById('d-ramp-pct-txt');
      if(ptxt) ptxt.textContent = pct+'%';
      var rtim = document.getElementById('d-ramp-time');
      if(rtim) rtim.textContent = timeStr;
      var rdir = document.getElementById('d-ramp-dir');
      if(rdir) rdir.innerHTML = d.rampUp ? '&#9650;' : '&#9660;';
    })();

    // ── HARMONOGRAM DNIA ──
    (function(){
      var sc     = d.schedule || {};
      var nowMin = (new Date()).getHours()*60 + (new Date()).getMinutes();
      var tln    = document.getElementById('d-tl-now');
      if(tln) tln.style.left = (nowMin/1440*100).toFixed(2)+'%';

      var mWD    = sc.morningWD    !== undefined ? sc.morningWD    : 7*60;
      var mWE    = sc.morningWE    !== undefined ? sc.morningWE    : 8*60;
      var midOff = sc.middayOff    !== undefined ? sc.middayOff    : 11*60;
      var sunMin = sc.sunsetMin    !== undefined ? sc.sunsetMin    : 19*60+32;
      var evBef  = sc.eveningBefore!== undefined ? sc.eveningBefore: -60;
      var evOff  = sc.eveningOff   !== undefined ? sc.eveningOff   : 21*60+30;
      var fade   = (d.fadeMinutes && d.fadeMinutes>0) ? parseInt(d.fadeMinutes) : 60;
      var isWE   = (new Date()).getDay()===0||(new Date()).getDay()===6;
      var morStart = isWE ? mWE : mWD;
      var evStart  = ((sunMin+evBef)+1440)%1440;
      var evOffEnd = evOff+fade;

      function mT(m){m=((m%1440)+1440)%1440;return String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0');}
      function inPhase(s,e){e=((e%1440)+1440)%1440;s=((s%1440)+1440)%1440;return e>s?(nowMin>=s&&nowMin<e):(nowMin>=s||nowMin<e);}

      // Timeline bar — rysuj po layoucie (offsetWidth=0 przy pierwszym renderze)
      setTimeout(function(){
        var c = document.getElementById('d-tl-canvas');
        if (!c) return;
        var parent = c.parentElement;
        var W = parent ? parent.offsetWidth : 0;
        if (W < 10) W = 380;
        c.width = W; c.height = 16;
        var ctx = c.getContext('2d');

        function px(m){ return m/1440*W; }

        if (!CanvasRenderingContext2D.prototype.roundRect) {
          CanvasRenderingContext2D.prototype.roundRect = function(x,y,w,h,r){
            this.beginPath();
            this.moveTo(x+r,y);this.lineTo(x+w-r,y);this.quadraticCurveTo(x+w,y,x+w,y+r);
            this.lineTo(x+w,y+h-r);this.quadraticCurveTo(x+w,y+h,x+w-r,y+h);
            this.lineTo(x+r,y+h);this.quadraticCurveTo(x,y+h,x,y+h-r);
            this.lineTo(x,y+r);this.quadraticCurveTo(x,y,x+r,y);this.closePath();return this;
          };
        }

        // Tło
        ctx.fillStyle = 'rgba(255,255,255,0.06)';
        ctx.roundRect(0,0,W,16,7); ctx.fill();

        // Segmenty w kolorach etapów — clip do zaokrąglonego prostokąta
        ctx.save();
        ctx.beginPath(); ctx.roundRect(0,0,W,16,7); ctx.clip();

        var minLuxColor = (d.minLuxActive) ? '#4ade80' : '#5a7a99';
        var segs = [
          [morStart,    midOff,      '#ff9f43',    0.85],
          [midOff,      midOff+fade, '#ff9f43',    0.45],
          [midOff+fade, evStart,     minLuxColor,  0.40],
          [evStart,     evOff,       '#ffd080',    0.85],
          [evOff,       evOffEnd,    '#ff4d6d',    0.55],
        ];
        segs.forEach(function(sg){
          var x1=px(sg[0]), x2=px(sg[1]);
          if(x2<=x1) return;
          ctx.globalAlpha=sg[3];
          ctx.fillStyle=sg[2];
          ctx.fillRect(x1,0,x2-x1,16);
        });
        ctx.globalAlpha=1;
        ctx.restore();

        // Linia teraz
        var tln=document.getElementById('d-tl-now');
        if(tln) tln.style.left=(nowMin/1440*100).toFixed(2)+'%';
      }, 100);

      // 5 phases
      var phases=[
        {
          id:1, name:'Rano', dot:'#ff9f43',
          s:morStart, e:midOff,
          act:inPhase(morStart,midOff),
          rampUp:fade, rampDown:null,
          sub:'<span class="'+(isWE?'ph-wd':'ph-wd-active')+'">PN\u2013PT: '+mT(mWD)+'</span>'
             +' &middot; <span class="'+(isWE?'ph-we-active':'ph-we')+'">SOB\u2013ND: '+mT(mWE)+'</span>'
        },
        {
          id:2, name:'Po\u0142udnie \u2013 wygaszanie', dot:'#ff9f43',
          s:midOff, e:midOff+fade,
          act:inPhase(midOff, midOff+fade),
          rampUp:null, rampDown:fade,
          sub:'koniec świecenia porannego'
        },
        {
          id:3, name:'Przerwa po\u0142udn.', dot:(d.minLuxActive ? '#4ade80' : '#5a7a99'),
          s:midOff+fade, e:evStart,
          act:inPhase(midOff+fade,evStart),
          rampUp:null, rampDown:null,
          sub:(function(){
            var en  = d.minLuxEnabled;
            var act = d.minLuxActive;
            var tgt = d.minLuxTarget;
            if(en && act)  return '\ud83c\udf31 Tryb lux mini aktywny \u2013 cel: '+(tgt||'?')+' lx';
            if(en && !act) return '\ud83c\udf31 Tryb lux mini gotowy \u2013 cel: '+(tgt||'?')+' lx (wystarczy świat\u0142a)';
            return 'LED wy\u0142\u0105czone';
          })()
        },
        {
          id:4, name:'\u015awiecenie wiecz.', dot:'#ffd080',
          s:evStart, e:evOff,
          act:inPhase(evStart,evOff),
          rampUp:fade, rampDown:null,
          sub:'offset '+(evBef>=0?'+':'')+evBef+' min od zachodu ('+mT(sunMin)+')'
        },
        {
          id:5, name:'Zach\u00f3d \u2013 wygaszanie', dot:'#ff4d6d',
          s:evOff, e:evOffEnd,
          act:inPhase(evOff,evOffEnd),
          rampUp:null, rampDown:fade,
          sub:'rampa \u2193 '+fade+' min \u2192 wy\u0142\u0105czenie'
        }
      ];

      var pl=document.getElementById('d-phase-list');
      if(!pl) return;
      var activePhase='';
      var html='';
      phases.forEach(function(p,i){
        if(p.act) activePhase=p.name;
        // Typ wiersza: on=pełne świecenie, ramp-up=wznoszenie, ramp-down=opadanie, break=przerwa, off=noc
        var typeClass = p.id===3 ? 'ph-type-break' : '';
        // hex dot -> rgba helper
        function dotRgba(hex, a) {
          var r=parseInt(hex.slice(1,3),16), g=parseInt(hex.slice(3,5),16), b=parseInt(hex.slice(5,7),16);
          return 'rgba('+r+','+g+','+b+','+a+')';
        }
        var dc = p.dot;
        var borderCol  = dotRgba(dc, p.act ? 0.90 : 0.55);
        var bgCol      = dotRgba(dc, p.act ? 0.10 : 0.04);
        var borderEdge = dotRgba(dc, p.act ? 0.40 : 0.15);
        var rowStyle   = 'border-left-color:'+borderCol+';background:'+bgCol+';border-color:'+borderEdge+';border-left-width:3px';
        if (p.act) rowStyle += ';box-shadow:0 2px 20px '+dotRgba(dc,0.18);
        html+='<div class="phase-row '+typeClass+(p.act?' ph-active':'')+'" style="'+rowStyle+'">';
        html+='<div class="ph-dot'+(p.act?' pulse':'')+'" style="background:'+dc+';'+(p.act?'box-shadow:0 0 7px '+dc+',0 0 14px '+dotRgba(dc,0.5)+';width:10px;height:10px;margin-top:4px':'box-shadow:0 0 4px '+dotRgba(dc,0.4))+'"></div>';
        html+='<div class="ph-content">';
        html+='<div class="ph-name" style="color:'+(p.act?'#fff':dc)+'">'+p.name+'</div>';
        html+='<div class="ph-sub">'+p.sub+'</div>';
        if(p.act) html+='<span class="ph-now-tag" style="background:'+dotRgba(dc,0.2)+';color:'+dc+';border-color:'+dotRgba(dc,0.5)+'">TERAZ</span>';
        html+='</div>';
        html+='<div class="ph-right">';
        html+='<div class="ph-times" style="color:'+(p.act?'#fff':dc)+'">'+mT(p.s)+'<span class="td"> \u2013 </span>'+mT(p.e%1440)+'</div>';
        var rbStyle = 'background:'+dotRgba(dc,0.18)+';color:'+dc+';border:1px solid '+dotRgba(dc,0.5)+';box-shadow:0 0 8px '+dotRgba(dc,0.2);
        if(p.rampUp)   html+='<span class="ramp-badge" style="'+rbStyle+'">\u2191 rampa '+p.rampUp+' min</span>';
        if(p.rampDown) html+='<span class="ramp-badge" style="'+rbStyle+'">\u2193 rampa '+p.rampDown+' min</span>';
        html+='</div>';
        html+='</div>';
        if(i<4) html+='<div class="ph-sep"></div>';
      });
      pl.innerHTML=html;

      var bdg=document.getElementById('d-sched-badge');
      var bph=document.getElementById('d-sched-phase');
      if(bdg&&bph){
        if(activePhase){bph.textContent=activePhase;bdg.style.display='';}
        else{bdg.style.display='none';}
      }
    })();

    // ── ENERGIA & CZAS ──
    (function(){
      var price = parseFloat((document.getElementById('d-kwh-price')||{}).value) || _kwhPrice || 0.80;
      var priceEl = document.getElementById('d-kwh-price');
      // [FIX-v3-KWH-INPUT] Poprzednio priceEl.value było nadpisywane bezwarunkowo
      // przy KAŻDYM odświeżeniu (co 3s z setInterval ORAZ przy każdym oninput
      // przez dUpdatePrice()->loadDash()). Flaga el._userEdited była ustawiana
      // w dUpdatePrice(), ale nigdzie nie była sprawdzana - pole i tak wracało
      // do starej wartości z localStorage/ESP zanim użytkownik zdążył wpisać nową.
      // Teraz: pomijamy nadpisanie, gdy pole ma fokus (user właśnie pisze) lub
      // gdy _userEdited==true (edycja w toku, jeszcze niezapisana onblur/onchange).
      if(priceEl && document.activeElement !== priceEl && !priceEl._userEdited) {
        // Priorytet: localStorage > ESP (ESP moze miec domyslne 0.80 po restarcie)
        var savedLocal = null;
        try{ savedLocal = localStorage.getItem('kwhPrice'); }catch(e){}
        if(savedLocal) {
          priceEl.value = parseFloat(savedLocal).toFixed(2);
          _kwhPrice = parseFloat(savedLocal);
          // Jesli ESP ma inna wartosc - zapisz do EEPROM cicho
          if(d.kwhPrice && Math.abs(parseFloat(d.kwhPrice)-parseFloat(savedLocal))>0.001){
            fetch(BASE+'/set-kwh-price?v='+parseFloat(savedLocal).toFixed(2)).catch(function(){});
          }
        } else if(d.kwhPrice) {
          priceEl.value = parseFloat(d.kwhPrice).toFixed(2);
          _kwhPrice = parseFloat(d.kwhPrice);
        }
      }
      var todayWh  = parseFloat(d.energyTodayWh)  || 0;
      var weekWh   = parseFloat(d.energyWeekWh)   || 0;
      var monthWh  = parseFloat(d.energyMonthWh)  || 0;
      var ledMin   = parseInt(d.ledOnMinutesToday) || 0;
      var ledMinWk = parseInt(d.ledOnMinutesWeek)  || 0;
      var ledMinMo = parseInt(d.ledOnMinutesMonth) || 0;
      var derat    = parseFloat(d.statSredniaRedukcja) || 0;
      var totW2    = parseFloat(d.powerNowW) || 0;
      var uptimeSec= Math.round(parseFloat(d.uptimeSec||0));

      function wToKwh(wh){return wh.toFixed(1);}
      function wToPln(wh){return (wh/1000*price).toFixed(2);}
      function ledTime(min){var h=Math.floor(min/60),m=min%60;return h+'h '+String(m).padStart(2,'0')+'m';}
      function upStr(sec){if(!sec)return '—';var d2=Math.floor(sec/86400),h2=Math.floor((sec%86400)/3600);return d2>0?d2+'d '+h2+'h':h2+'h '+Math.floor((sec%3600)/60)+'m';}

      // ─ DZIS ─
      var ep=document.getElementById('d-en-pow');         if(ep)  ep.textContent=totW2.toFixed(1);
      var et=document.getElementById('d-en-today');       if(et)  et.textContent=wToKwh(todayWh);
      var ect=document.getElementById('d-en-cost-today'); if(ect) ect.textContent=wToPln(todayWh);
      var elt=document.getElementById('d-en-ledtime');    if(elt) elt.textContent=ledTime(ledMin);
      // Zaoszczędzona energia dziś dzięki słońcu:
      // Jeśli średnia redukcja = R%, to LED zużył (100-R)% pełnej mocy.
      // Zaoszcz = energyTodayWh * R / (100 - R)
      var esvEl = document.getElementById('d-en-saved');
      if(esvEl) {
        var savedWh = (derat > 0 && derat < 100) ? todayWh * derat / (100 - derat) : 0;
        esvEl.textContent = savedWh.toFixed(1);
      }
      // Aktywacje MIN LUX dziś
      var emlEl = document.getElementById('d-en-minlux-act');
      if(emlEl) emlEl.textContent = (d.minLuxActivToday || 0);
      // Szczytowa moc dziś (max z historii lub bieżąca jeśli wyższa)
      var peakW = parseFloat(d.peakPowerWToday) || parseFloat(d.powerNowW) || 0;
      var epkEl = document.getElementById('d-en-peak');
      if(epkEl) epkEl.textContent = peakW.toFixed(1);
      // Lux·godziny przy rośblinach dziś
      var elxEl = document.getElementById('d-en-luxh');
      if(elxEl) {
        var luxH = parseFloat(d.luxHoursTodayWater) || 0;
        elxEl.textContent = luxH >= 1000 ? (luxH/1000).toFixed(1) + 'k' : Math.round(luxH);
      }

      // Sparkline moc 12h
      (function(){
        var sc=document.getElementById('d-spark-day'); if(!sc)return;
        fetch(BASE+'/api/history').then(function(r){return r.text();}).then(function(csv){
          var lines=csv.trim().split('\n'),hdr=(lines[0]||'').toLowerCase().split(',');
          // [OK] FIX-v29-10: brak power_w w CSV - oblicz moc z PWM (5 kanałów * Wmax/1023)
          var pIdx=hdr.indexOf('power_w');
          var pwmIdxs=[hdr.indexOf('pwm0_biale'),hdr.indexOf('pwm1_fs'),hdr.indexOf('pwm2_fs_biale'),hdr.indexOf('pwm3_nieb'),hdr.indexOf('pwm4_czerw')];
          var usePwm=(pIdx<0)&&pwmIdxs[0]>=0;
          var LED_MAX_W=33.0;
          var rows=lines.slice(1).filter(function(l){return l.trim();}).slice(-144);
          var bc=12,step=Math.max(1,Math.floor(rows.length/bc)),bkts=[];
          for(var ii=0;ii<rows.length;ii+=step){var sl=rows.slice(ii,ii+step),ss=0,cc=0;
            sl.forEach(function(l){var cols=l.split(','),v;
              if(!usePwm){v=parseFloat(cols[pIdx]);}
              else{var tot=0;pwmIdxs.forEach(function(pi){if(pi>=0)tot+=parseFloat(cols[pi])||0;});v=tot/5/1023*LED_MAX_W;}
              if(!isNaN(v)){ss+=v;cc++;}});
            if(cc) bkts.push(ss/cc);}
          if(!bkts.length)return;
          var mx=Math.max.apply(null,bkts)||1,h='';
          bkts.forEach(function(v,i){
            var cls='sp-bar'+(i===bkts.length-1?' hi':'');
            h+='<div class="'+cls+'" style="height:'+(v/mx*100).toFixed(1)+'%" title="'+v.toFixed(1)+'W"></div>';
          });
          sc.innerHTML=h;
        }).catch(function(){});
      })();

      // ─ TYDZIEN ─
      var ew=document.getElementById('d-en-week');        if(ew)  ew.textContent=wToKwh(weekWh)+' Wh';
      var ecw=document.getElementById('d-en-cost-week');  if(ecw) ecw.textContent=wToPln(weekWh)+' zł';
      // [OK] FIX: rzeczywisty czas tygodniowy z akumulatora ledOnMinutesWeek
      var eltw=document.getElementById('d-en-ledtime-week'); if(eltw) eltw.textContent=ledTime(ledMinWk);
      // [OK] FIX: średnia tygodniowa dzielona przez liczbę dni bieżącego tygodnia (Pn=1..Nd=7)
      var daysInWeekSoFar = Math.max(1, (new Date().getDay() + 6) % 7 + 1);
      var avgWh=weekWh/daysInWeekSoFar;
      var eav=document.getElementById('d-en-avg');        if(eav) eav.textContent=wToKwh(avgWh);
      var ecav=document.getElementById('d-en-cost-avg');  if(ecav) ecav.textContent=wToPln(avgWh);
      // [OK] FIX: średni dzienny czas LED w tygodniu
      var elav=document.getElementById('d-en-ledtime-avg'); if(elav) elav.textContent=ledTime(Math.round(ledMinWk/daysInWeekSoFar));
      // Zaoszczędzone Wh w tygodniu dzięki słońcu
      var eswk = document.getElementById('d-en-saved-week');
      if(eswk) {
        var savedWhWk = (derat > 0 && derat < 100) ? weekWh * derat / (100 - derat) : 0;
        eswk.textContent = savedWhWk.toFixed(1);
      }
      // śr. aktywacje MIN LUX na dzień
      var emla = document.getElementById('d-en-minlux-avg');
      if(emla) {
        var mlWkTotal = parseFloat(d.minLuxActivWeek) || 0;
        emla.textContent = (mlWkTotal / daysInWeekSoFar).toFixed(1);
      }
      // Prognoza koszt roczny (na bazie średniej dziennej z tygodnia)
      var eyr = document.getElementById('d-en-cost-year');
      if(eyr) {
        var yearCost = avgWh * 365 / 1000 * price;
        eyr.textContent = yearCost.toFixed(2);
      }
      // Wykres tygodniowy - słupki od poniedziałku do dziś (rzeczywiste), reszta szara
      (function(){
        var wb=document.getElementById('d-week-bars'); var wl=document.getElementById('d-week-labels');
        if(!wb) return;
        var days=['Pn','Wt','Śr','Cz','Pt','Sb','Nd'];
        // [OK] FIX-v29-11: użyj realnej historii dni z firmware (d.dayHistory[0..6] = Pn..Nd)
        var hist = (d.dayHistory && d.dayHistory.length===7) ? d.dayHistory : [0,0,0,0,0,0,0];
        var isoToday = (new Date().getDay() + 6) % 7;
        var vals=[], dayOrder=[];
        for(var dd=0; dd<7; dd++){
          if(dd < isoToday) vals.push(hist[dd] || 0);    // poprzednie dni: realne z historii
          else if(dd === isoToday) vals.push(todayWh);   // dziś: live
          else vals.push(0);                             // przyszłe: 0
          dayOrder.push(dd);
        }
        var mx=Math.max.apply(null,vals)||1, h='', hl='';
        var isoToday2=(new Date().getDay()+6)%7;
        vals.forEach(function(v,i){
          var cls='mb'+(i===isoToday2?' today':i<isoToday2&&v===mx?' hi':'');
          var tip = i<isoToday2 ? '~'+v.toFixed(1)+' Wh (śred.)' : i===isoToday2 ? v.toFixed(1)+' Wh (dziś)' : '—';
          h+='<div class="'+cls+'" style="height:'+(i<=isoToday2&&v>0?(v/mx*100).toFixed(1):4)+'%;opacity:'+(i>isoToday2?'0.2':'1')+'" title="'+tip+'"></div>';
          hl+='<span>'+days[i]+'</span>';
        });
        wb.innerHTML=h; if(wl) wl.innerHTML=hl;
      })();

      // ─ MIESIAC ─
      var em=document.getElementById('d-en-month');       if(em)  em.textContent=wToKwh(monthWh)+' Wh';
      var ecm=document.getElementById('d-en-cost-month'); if(ecm) ecm.textContent=wToPln(monthWh)+' zł';
      var daysInMon=new Date(new Date().getFullYear(),new Date().getMonth()+1,0).getDate();
      var dayOfMon=new Date().getDate();
      // [OK] FIX: rzeczywisty miesięczny czas LED z akumulatora ledOnMinutesMonth
      var eltm=document.getElementById('d-en-ledtime-month'); if(eltm) eltm.textContent=ledTime(ledMinMo);
      var eavm=document.getElementById('d-en-avg-month'); if(eavm) eavm.textContent=wToKwh(monthWh/Math.max(dayOfMon,1));
      var forecastWh=dayOfMon>0?(monthWh/dayOfMon*daysInMon):monthWh;
      var efc=document.getElementById('d-en-forecast');   if(efc) efc.textContent='~'+wToKwh(forecastWh);
      var ecfc=document.getElementById('d-en-cost-forecast'); if(ecfc) ecfc.textContent='~'+wToPln(forecastWh);
      // Oszczędność zł dzięki adaptacji (w miesiącu)
      var esmEl = document.getElementById('d-en-saved-month');
      if(esmEl) {
        var savedWhMpln = (derat > 0 && derat < 100) ? monthWh * derat / (100 - derat) : 0;
        esmEl.textContent = (savedWhMpln / 1000 * price).toFixed(2);
      }
      // Energia jaką by zużył bez adaptacji słońca
      var enaEl = document.getElementById('d-en-no-adapt');
      if(enaEl) {
        var savedWhM2 = (derat > 0 && derat < 100) ? monthWh * derat / (100 - derat) : 0;
        enaEl.textContent = (monthWh + savedWhM2).toFixed(1);
      }
      // [OK] FIX: rzeczywisty łączny czas świecenia w miesiącu
      var eltmEl = document.getElementById('d-en-ledtime-total');
      if(eltmEl) eltmEl.textContent = ledTime(ledMinMo);
      // Szczytowa moc (z sesji przeglądarki)
      var epkMEl = document.getElementById('d-en-peak');
      if(epkMEl) epkMEl.textContent = (parseFloat(d.peakPowerWToday) || parseFloat(d.powerNowW) || 0).toFixed(1);
      // Wykres miesieczny
      (function(){
        var mb=document.getElementById('d-month-bars'); if(!mb) return;
        // [OK] FIX-v29-11: użyj realnej historii tygodni z firmware (d.weekHistory[0..4])
        var wHist = (d.weekHistory && d.weekHistory.length===5) ? d.weekHistory : [0,0,0,0,0];
        var curWkIdx = Math.min(4, Math.max(0, Math.floor((dayOfMon - 1) / 7)));
        var vals4 = [
          curWkIdx >= 1 ? (wHist[0] || 0) : 0,
          curWkIdx >= 2 ? (wHist[1] || 0) : 0,
          curWkIdx >= 3 ? (wHist[2] || 0) : 0,
          weekWh  // bieżący tydzień live
        ];
        var mx=Math.max.apply(null,vals4)||1, h='', hl='';
        var lbls=['T1','T2','T3','T4'];
        vals4.forEach(function(v,i){
          var isLast=(i===3), hasData=(v>0);
          var cls='mb'+(isLast?' today':(hasData&&v===mx?' hi':''));
          var ht=(hasData||isLast)?(v/mx*100).toFixed(1):'4';
          var op=(!hasData&&!isLast)?'opacity:0.2;':'';
          var tip=hasData?v.toFixed(1)+' Wh'+(isLast?' (bieżący)':'') :'—';
          h+='<div class="'+cls+'" style="height:'+ht+'%;'+op+'" title="'+lbls[i]+': '+tip+'"></div>';
          hl+='<span>'+lbls[i]+'</span>';
        });
        mb.innerHTML=h;
        var ml=document.getElementById('d-month-labels'); if(ml) ml.innerHTML=hl;
      })();

    })();

  }).catch(function(e){console.log('Dashboard blad:',e);});
}

// ── DASHBOARD HELPERS ──
var _dActivePeriod = 'day';
function dSaveKwhPrice(inp){
  var v = parseFloat(inp.value);
  if(isNaN(v)||v<0.01||v>99) return;
  inp._userEdited = false;  // [FIX-v3-KWH-INPUT] edycja zakończona - pozwól znów synchronizować pole
  // Zapisz lokalnie natychmiast (niezaleznie od ESP)
  try{ localStorage.setItem('kwhPrice', v.toFixed(2)); }catch(e){}
  _kwhPrice = v;
  fetch(BASE+'/set-kwh-price?v='+v.toFixed(2))
    .then(function(r){
      if(r.ok){
        _kwhPrice = v;
        // pokaz ikone zapisu
        var ic = document.getElementById('d-kwh-saved-icon');
        if(ic){
          ic.style.color='var(--green)';
          ic.style.opacity='1';
          inp.style.borderColor='rgba(74,222,128,.5)';
          setTimeout(function(){
            ic.style.opacity='0';
            inp.style.borderColor='';
          }, 2500);
        }
      } else {
        var ic2 = document.getElementById('d-kwh-saved-icon');
        if(ic2){ ic2.style.color='var(--red)'; ic2.textContent='\u2715'; ic2.style.opacity='1';
          setTimeout(function(){ ic2.style.opacity='0'; ic2.textContent='\u2713'; },2500); }
      }
    }).catch(function(){
      var ic3 = document.getElementById('d-kwh-saved-icon');
      if(ic3){ ic3.style.color='var(--red)'; ic3.textContent='\u2715'; ic3.style.opacity='1';
        setTimeout(function(){ ic3.style.opacity='0'; ic3.textContent='\u2713'; },2500); }
    });
}
function dSetEPeriod(p,btn){
  var par=btn.closest('.card-body')||btn.parentElement.parentElement;
  par.querySelectorAll('.ptab').forEach(function(b){b.classList.remove('on');});
  par.querySelectorAll('.period-panel').forEach(function(pp){pp.classList.remove('on');});
  btn.classList.add('on');
  var panel=document.getElementById('dep-'+p); if(panel) panel.classList.add('on');
}
function dSetPeriod(p, btn) {
  _dActivePeriod = p;
  ['day','week','month'].forEach(function(id){
    var panel = document.getElementById('d-panel-'+id);
    var tab   = document.getElementById('d-tab-'+id);
    if(panel) panel.style.display = (id===p) ? '' : 'none';
    if(tab){
      tab.style.border   = (id===p) ? '1px solid rgba(0,212,245,.4)' : '1px solid var(--border)';
      tab.style.color    = (id===p) ? 'var(--cyan)' : 'var(--text-dim)';
      tab.style.background = (id===p) ? 'rgba(0,212,245,.1)' : 'transparent';
    }
  });
}
function dUpdatePrice(val) {
  var el = document.getElementById('d-kwh-price');
  if(el) el._userEdited = true;
  // [FIX-v3-KWH-INPUT] usunięto loadDash() - wywoływanie pełnego przeładowania
  // dashboardu przy KAŻDYM naciśnięciu klawisza powodowało wyścig z odświeżaniem
  // pola ceny (patrz FIX-v3-KWH-INPUT wyżej). Podgląd "Prognoza koszt/rok" i tak
  // przeliczy się przy najbliższym cyklicznym loadDash() (co 3s) lub po onblur.
}

// ══════════════ TERMINAL JS - pełna wersja ══════════════
var ws, autoS=true, showTS=true, tColorize=true, tPaused=false, tAudio=false;
var pingEnabled=true, pingTimer=null;
var tLines=[], hist=[], hi=-1;
var tCat='all', tFilter='', tSearch='', tFMode='text', tErrCount=0;
var tCounts={all:0,err:0,warn:0,temp:0,led:0,ramp:0,pump:0,adapt:0,ntp:0};
var tStatusTimer=null;
var T=null;

function tGetT(){if(!T)T=document.getElementById('term');return T;}

function tClassify(txt){
  var t=txt.toLowerCase(),cls='',cat='all';
  // BŁĘDY
  if(/[ERR]|🚨|awaria|brak czasu ntp|brak wifi|littlefs błąd|crash|critical|heap krytyczny/.test(t)){cls='err';cat='err';tErrCount++;}
  // OSTRZEŻENIA
  else if(/[WARN]️|warn|saturacja ir|czujnik zwrócił 0|brak połączenia|cicha zmiana|diag-/.test(t)){cls='warn';cat='warn';}
  // OK
  else if(/[OK]|ok|gotowe|połączono/.test(t)){cls='ok';}
  // TEMPERATURY
  if(/ds18b20|temp płyta|płyta1|płyta2|strefa1|strefa2|derating|redukcja mocy|przegrzanie|°c|woda.*błąd/.test(t)){cls=cls||'t-temp';cat=cat==='all'?'temp':cat;}
  // RAMPY
  if(/miękki start|przejście auto|przejście manualne|przejście manual->auto|przejście auto->manual|rampa start|rampa koniec|mid-ramp|dobieg|fade|soft.start|transition|wznosz|opadan/.test(t)){cls=cls||'t-ramp';cat=cat==='all'?'ramp':cat;}
  // POMPA
  if(/pompa|pump|💦/.test(t)){cat=cat==='all'?'pump':cat;}
  // ADAPTACJA
  if(/adapt|🌿|min lux|tsl|lux |transmis|uczeni|korekta min|korekta max|redukcja led|⚖️|🔆|🧠|próbek/.test(t)){cls=cls||'t-adapt';cat=cat==='all'?'adapt':cat;}
  // NTP / SIEĆ
  if(/ntp|wifi|cloud|mqtt|połączono|synchron|zachód słońca|sunset|strefa czas|iot|chmura/.test(t)){cat=cat==='all'?'ntp':cat;}
  // LED/PWM
  if(/pwm|led|jasność|brightness|flash|backupbright|diod|kanał|zasilanie|wybór.*led/.test(t)){cat=cat==='all'?'led':cat;}
  return{cls:cls,cat:cat};
}

function addLine(txt,forceCls){
  var cl=tClassify(txt);
  var cls=forceCls!==undefined?forceCls:(tColorize?cl.cls:'');
  var cat=cl.cat;
  tCounts.all++;if(tCounts[cat]!==undefined)tCounts[cat]++;
  tUpdateCounts();
  if(cls==='err'&&tAudio)tBeep();
  var now=new Date();
  var ts=String(now.getHours()).padStart(2,'0')+':'+String(now.getMinutes()).padStart(2,'0')+':'+String(now.getSeconds()).padStart(2,'0');
  var entry={ts:ts,txt:txt,cls:cls,cat:cat};
  tLines.push(entry);if(tLines.length>5000)tLines.shift();
  var dom=tBuildLine(entry);
  if(!tLineVis(entry))dom.style.display='none';
  var TT=tGetT();
  var empty=TT.querySelector('#t-empty');if(empty)empty.remove();
  TT.appendChild(dom);
  while(TT.children.length>2000)TT.removeChild(TT.firstChild);
  var lc=document.getElementById('t-lcnt');if(lc)lc.textContent=tCounts.all+' linii';
  if(tErrCount>0){var ec=document.getElementById('t-ecnt');if(ec){ec.textContent=tErrCount+' b\u0142\u0119d\u00f3w';ec.classList.add('vis');}}
  var sh=document.getElementById('t-scroll-hint');
  if(autoS){TT.scrollTop=TT.scrollHeight;if(sh)sh.classList.remove('vis');}
  else{if(sh)sh.classList.add('vis');}
}

function tBuildLine(e){
  var d=document.createElement('div');
  d.className='ln'+(e.cls?' '+e.cls:'');
  d.dataset.cat=e.cat;d.dataset.txt=e.txt.toLowerCase();
  if(showTS){var s=document.createElement('span');s.className='ts';s.textContent='['+e.ts+'] ';d.appendChild(s);}
  var l=document.createElement('span');l.className='lntxt';
  if(tSearch){l.innerHTML=tHighlight(e.txt,tSearch);}else{l.textContent=e.txt;}
  d.appendChild(l);return d;
}

function tHighlight(txt,q){
  var esc=q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&');
  try{return txt.replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(new RegExp('('+esc+')','gi'),'<span class="t-hl">$1</span>');}
  catch(e){return txt;}
}

function tLineVis(e){
  if(tCat!=='all'&&e.cat!==tCat)return false;
  if(tFilter){
    if(tFMode==='regex'){try{if(!(new RegExp(tFilter,'i').test(e.txt)))return false;}catch(ex){return false;}}
    else{if(e.txt.toLowerCase().indexOf(tFilter.toLowerCase())<0)return false;}
  }
  return true;
}

function tRerender(){
  var TT=tGetT();var nodes=TT.querySelectorAll('.ln');
  for(var i=0;i<nodes.length;i++){
    var n=nodes[i];var cat=n.dataset.cat||'all';var txt=n.dataset.txt||'';var vis=true;
    if(tCat!=='all'&&cat!==tCat)vis=false;
    if(tFilter&&vis){
      if(tFMode==='regex'){try{if(!(new RegExp(tFilter,'i').test(txt)))vis=false;}catch(ex){vis=false;}}
      else{if(txt.indexOf(tFilter.toLowerCase())<0)vis=false;}
    }
    n.style.display=vis?'':'none';
    if(vis){var span=n.querySelector('.lntxt');if(span){var orig=tLines.find(function(l){return l.txt.toLowerCase()===txt;});if(orig){if(tSearch){span.innerHTML=tHighlight(orig.txt,tSearch);}else{span.textContent=orig.txt;}}}}
  }
}

function tUpdateCounts(){Object.keys(tCounts).forEach(function(k){var el=document.getElementById('tcc-'+k);if(el)el.textContent=tCounts[k];});}

function connect(){
  if(ws&&ws.readyState===WebSocket.OPEN)return;
  var wsUrl='ws://'+HOST+':'+PORT+'/ws';
  addLine('-> Łączę z '+wsUrl+'...','');
  try{ws=new WebSocket(wsUrl);}catch(e){addLine('✗ '+e.message,'err');return;}
  ws.onopen=function(){
    addLine('✓ Połączono z ESP32','ok');
    document.getElementById('connlabel').textContent='Po\u0142\u0105czono';
    // ── NAGŁÓWEK SYSTEMOWY ──
    (function(){
      var ip = document.getElementById('sh-ip');
      var tm = document.getElementById('sh-time');
      var bw = document.getElementById('sh-wifi');
      var bn = document.getElementById('sh-ntp');
      var bm = document.getElementById('sh-mode');
      if(ip && d.ip) ip.textContent = d.ip;
      if(tm && d.localTime) tm.textContent = d.localTime;
      if(bw){ var ok=d.wifiOK!==false; bw.className='sys-badge '+(ok?'ok':'warn');
        bw.innerHTML='<div class="sbdot" style="background:'+(ok?'#4ade80':'#fbbf24')+'"></div>WiFi'; }
      if(bn){ var ok2=d.ntpOK!==false; bn.className='sys-badge '+(ok2?'ok':'warn');
        bn.innerHTML='<div class="sbdot" style="background:'+(ok2?'#4ade80':'#fbbf24')+'"></div>NTP'; }
      if(bm){ var isAuto=d.tryb===true||d.tryb==='true';
        bm.textContent=isAuto?'AUTO':'MANUAL';
        bm.style.background=isAuto?'rgba(0,212,245,.1)':'rgba(255,160,50,.1)';
        bm.style.borderColor=isAuto?'rgba(0,212,245,.3)':'rgba(255,160,50,.3)';
        bm.style.color=isAuto?'var(--cyan)':'#ffa032'; }
    })();
    document.getElementById('bdot').style.background='var(--cyan)';
    tStartStatus();
    tStartPing();
  };
  ws.onmessage=function(e){
    if(tPaused)return;
    e.data.split('\n').forEach(function(l){l=l.trim();if(l&&l!=='pong')addLine(l);});
  };
  ws.onclose=function(e){
    addLine('✗ Rozłączono (kod '+e.code+')','err');
    document.getElementById('connlabel').textContent='Roz\u0142\u0105czono';
    document.getElementById('bdot').style.background='var(--red)';
    tStopStatus();
    tStopPing();
    setTimeout(function(){if(!ws||ws.readyState!==WebSocket.OPEN)connect();},5000);
  };
  ws.onerror=function(){addLine('✗ Błąd WebSocket','err');};
}

function send(){var v=document.getElementById('ci').value.trim();if(!v)return;hist.unshift(v);hi=-1;if(ws&&ws.readyState===1){ws.send(v);addLine('> '+v,'ok');}document.getElementById('ci').value='';}
function ck(e){if(e.key==='Enter'){send();}else if(e.key==='ArrowUp'){hi=Math.min(hi+1,hist.length-1);if(hist[hi])document.getElementById('ci').value=hist[hi];}else if(e.key==='ArrowDown'){hi=Math.max(hi-1,-1);document.getElementById('ci').value=hi>=0?hist[hi]:'';}}
function wscmd(c){if(ws&&ws.readyState===1){ws.send(c);addLine('> '+c,'ok');}else{toast('Brak po\u0142\u0105czenia','err');}}
function toggleAS(){autoS=!autoS;var b=document.getElementById('asBtn');if(b)b.classList.toggle('on',autoS);if(autoS)tScrollBottom();}
function toggleTS(){showTS=!showTS;var b=document.getElementById('tsBtn');if(b)b.classList.toggle('on',showTS);tGetT().querySelectorAll('.ts').forEach(function(n){n.style.display=showTS?'':'none';});}
function tToggleColor(){tColorize=!tColorize;var b=document.getElementById('tColBtn');if(b)b.classList.toggle('on',tColorize);var TT=tGetT();TT.querySelectorAll('.ln').forEach(function(n,i){n.className=tColorize&&tLines[i]?'ln'+(tLines[i].cls?' '+tLines[i].cls:''):'ln';});}
function tTogglePause(){tPaused=!tPaused;var b=document.getElementById('tPauseBtn');if(b){b.classList.toggle('on',tPaused);b.innerHTML=tPaused?'&#9654; Wzn\u00f3w':'&#9208; Pauza';}if(tPaused)addLine('⏸ PAUZA');else addLine('▶ Wznowiono');}
function tToggleAudio(){tAudio=!tAudio;var b=document.getElementById('tAudBtn');if(b){b.classList.toggle('on',tAudio);b.innerHTML=tAudio?'&#128276; Alert':'&#128277; Alert';}}
function tToggleSidebar(){var s=document.getElementById('t-sidebar'),o=document.getElementById('t-sb-overlay'),b=document.getElementById('t-sb-toggle');s.classList.toggle('open');if(o)o.classList.toggle('vis');if(b)b.classList.toggle('on',s.classList.contains('open'));}
function tStartPing(){tStopPing();if(!pingEnabled)return;pingTimer=setInterval(function(){if(ws&&ws.readyState===1){ws.send('ping');}},30000);}
function tStopPing(){if(pingTimer){clearInterval(pingTimer);pingTimer=null;}}
function tTogglePing(){pingEnabled=!pingEnabled;var b=document.getElementById('tPingBtn');if(b){b.classList.toggle('on',pingEnabled);b.innerHTML=pingEnabled?'&#128241; Ping ON':'&#128241; Ping OFF';}if(pingEnabled){tStartPing();addLine('\u{1F4F1} Keepalive ping W\u0141\u0104CZONY (co 30s)','ok');}else{tStopPing();addLine('\u{1F4F1} Keepalive ping WY\u0141\u0104CZONY','warn');}}
function clearDisp(){tGetT().innerHTML='';tLines=[];tErrCount=0;Object.keys(tCounts).forEach(function(k){tCounts[k]=0;});tUpdateCounts();var lc=document.getElementById('t-lcnt');if(lc)lc.textContent='0 linii';var ec=document.getElementById('t-ecnt');if(ec)ec.classList.remove('vis');}
function tScrollBottom(){var TT=tGetT();TT.scrollTop=TT.scrollHeight;var sh=document.getElementById('t-scroll-hint');if(sh)sh.classList.remove('vis');}
function tExport(){var c=tLines.map(function(l){return'['+l.ts+'] '+l.txt;}).join('\n');var fn='esp32_log_'+new Date().toISOString().slice(0,19).replace(/[T:]/g,'-')+'.txt';var b=new Blob([c],{type:'text/plain'});var a=document.createElement('a');a.href=URL.createObjectURL(b);a.download=fn;a.click();toast('Eksport: '+fn,'ok');}
function tSetTheme(v){var l=document.getElementById('t-layout');if(l)l.className=v||'';}
function tUpdateBadge(){var b=document.getElementById('tsb-badge');if(!b)return;var hasFilter=(tFilter&&tFilter.length>0);var hasCat=(tCat&&tCat!=='all');if(hasCat&&hasFilter){b.textContent=tCat+' + filtr';b.classList.add('vis');}else if(hasCat){b.textContent=tCat;b.classList.add('vis');}else if(hasFilter){b.textContent='filtr';b.classList.add('vis');}else{b.classList.remove('vis');}}
function tSetCat(cat,el){tCat=cat;document.querySelectorAll('.t-chip').forEach(function(c){c.classList.remove('on');});el.classList.add('on');tUpdateBadge();tRerender();}
function tApplyFilter(){tFilter=document.getElementById('t-fi').value;var fi=document.getElementById('t-fi');if(fi)fi.classList.toggle('t-fi-active',!!tFilter);tUpdateBadge();tRerender();}
function tFilterKey(e){if(e.key==='Escape'){tFilter='';var fi=document.getElementById('t-fi');if(fi){fi.value='';fi.classList.remove('t-fi-active');}tRerender();}}
function tSetFMode(m){tFMode=m;var a=document.getElementById('tfm-text'),b=document.getElementById('tfm-regex');if(a)a.classList.toggle('on',m==='text');if(b)b.classList.toggle('on',m==='regex');tApplyFilter();}
function tApplySearch(){tSearch=document.getElementById('t-srch').value;tRerender();}
function tClearSearch(){tSearch='';var s=document.getElementById('t-srch');if(s)s.value='';tRerender();}
function tStartStatus(){tStopStatus();tFetchStatus();tStatusTimer=setInterval(tFetchStatus,7000);}
function tStopStatus(){if(tStatusTimer){clearInterval(tStatusTimer);tStatusTimer=null;}}
function tFetchStatus(){
  fetch(BASE+'/api/status').then(function(r){return r.json();}).then(function(d){
    var n=new Date(),age=document.getElementById('t-sage');
    if(age)age.textContent=String(n.getHours()).padStart(2,'0')+':'+String(n.getMinutes()).padStart(2,'0');
    function sv(id,val,cls){var el=document.getElementById(id);if(!el)return;el.textContent=val;el.className='t-stval'+(cls?' '+cls:'');}
    sv('tsv-power',d.power?'W\u0141':'WY\u0141',d.power?'s-on':'s-off');
    sv('tsv-tryb',d.tryb?'AUTO':'MANUAL',d.tryb?'s-auto':'s-manual');
    var t1=parseFloat(d.temps&&d.temps[0]),t2=parseFloat(d.temps&&d.temps[1]),tw=parseFloat(d.temps&&d.temps[2]);
    function tc(t){return t>60?'s-crit':t>45?'s-hot':'';}
    sv('tsv-t1',isNaN(t1)?'\u2014':t1.toFixed(1)+'\u00b0C',tc(t1));
    sv('tsv-t2',isNaN(t2)?'\u2014':t2.toFixed(1)+'\u00b0C',tc(t2));
    sv('tsv-tw',isNaN(tw)?'\u2014':tw.toFixed(1)+'\u00b0C');
    sv('tsv-lux',d.luxRoom?Math.round(d.luxRoom)+' lx':'\u2014');
    sv('tsv-pwm',d.pwm&&d.pwm[0]!==undefined?d.pwm[0]+' ('+Math.round(d.pwm[0]/1023*100)+'%)':'\u2014');
    sv('tsv-adapt',d.adaptEnabled?'W\u0141':'WY\u0141',d.adaptEnabled?'s-on':'s-off');
    sv('tsv-pump',d.pumpOn?'W\u0141 \ud83d\udca7':'WY\u0141',d.pumpOn?'s-on':'s-off');
  }).catch(function(){});
}
var _tACtx=null;
function tBeep(){try{if(!_tACtx)_tACtx=new(window.AudioContext||window.webkitAudioContext)();var o=_tACtx.createOscillator(),g=_tACtx.createGain();o.connect(g);g.connect(_tACtx.destination);o.frequency.value=880;g.gain.setValueAtTime(.2,_tACtx.currentTime);g.gain.exponentialRampToValueAtTime(.0001,_tACtx.currentTime+.3);o.start(_tACtx.currentTime);o.stop(_tACtx.currentTime+.3);}catch(e){}}
(function(){document.addEventListener('DOMContentLoaded',function(){var TT=document.getElementById('term');if(!TT)return;TT.addEventListener('scroll',function(){var atBot=(TT.scrollHeight-TT.scrollTop-TT.clientHeight)<40;if(atBot){var sh=document.getElementById('t-scroll-hint');if(sh)sh.classList.remove('vis');if(!autoS){autoS=true;var b=document.getElementById('asBtn');if(b)b.classList.add('on');}}else{autoS=false;var b=document.getElementById('asBtn');if(b)b.classList.remove('on');}});});})();
(function(){var s=document.createElement('style');s.textContent='#t-layout.t-amber{--bg:#0a0800;--surface:#120e00;--surface2:#1a1400;--border:rgba(255,160,0,.12);--border-h:rgba(255,200,0,.3);--cyan:#ffb300;--text:#d4b060;--text-dim:#5a4010}#t-layout.t-green{--bg:#000d05;--surface:#001a0a;--surface2:#002010;--border:rgba(0,180,60,.12);--border-h:rgba(0,220,80,.3);--cyan:#00e060;--text:#50c870;--text-dim:#105030}';document.head.appendChild(s);})();
// ── INIT ──
window.addEventListener('load', function(){
  PAGES.forEach(function(p){
    var el = document.getElementById('p-'+p);
    if (el) el.style.display = 'none';
  });
  var dash = document.getElementById('p-dash');
  if (dash) dash.style.display = 'block';
  loadDash();
  loadCharts();
  locLoad();  // [4.7.0 ASTRO] lokalizacja do zachodu
  setInterval(function(){
    var d=document.getElementById('p-dash');
    if(d&&d.style.display!=='none') loadDash();
  }, 3000);
});
// ── LOKALIZACJA / ZACHÓD SŁOŃCA [4.7.0 ASTRO] ──
function locMsg(txt,ok){
  var el=document.getElementById('loc-msg');
  if(el){el.textContent=txt;el.style.color=ok?'#22d3aa':'#ff4d6d';}
}
function locLoad(){
  fetch(BASE+'/api/location')
  .then(function(r){return r.json();})
  .then(function(d){
    var la=document.getElementById('loc-lat');
    var lo=document.getElementById('loc-lon');
    var sn=document.getElementById('loc-sunset');
    if(la&&document.activeElement!==la) la.value=Number(d.lat).toFixed(4);
    if(lo&&document.activeElement!==lo) lo.value=Number(d.lon).toFixed(4);
    if(sn) sn.textContent=d.sunset+(d.timeSynced?'':' (brak czasu NTP, warto\u015b\u0107 domy\u015blna)');
  })
  .catch(function(){locMsg('Blad polaczenia',false);});
}
function locSave(){
  var la=parseFloat((document.getElementById('loc-lat')||{}).value);
  var lo=parseFloat((document.getElementById('loc-lon')||{}).value);
  if(isNaN(la)||isNaN(lo)||la<-90||la>90||lo<-180||lo>180){
    locMsg('Szeroko\u015b\u0107 -90..90, d\u0142ugo\u015b\u0107 -180..180',false);return;
  }
  fetch(BASE+'/api/location',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({lat:la,lon:lo})})
  .then(function(r){return r.json();})
  .then(function(d){
    locMsg(d.ok?'Zapisano! Zach\u00f3d zostanie przeliczony.':(d.error||'Blad zapisu'),d.ok);
    if(d.ok) setTimeout(locLoad,1500);
  })
  .catch(function(){locMsg('Blad polaczenia',false);});
}
// ── TELEGRAM ──
function tgMsg(txt,ok){
  var el=document.getElementById('tg-msg');
  if(el){el.textContent=txt;el.style.color=ok?'#22d3aa':'#ff4d6d';}
}
function tgSave(){
  var tok=(document.getElementById('tg-token')||{}).value||'';
  var cid=(document.getElementById('tg-chatid')||{}).value||'';
  var en=(document.getElementById('tg-enabled')||{}).checked||false;
  if(!tok&&!cid){tgMsg('Wypelnij token i chat ID',false);return;}
  fetch(BASE+'/api/telegram/save',{method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({token:tok,chatId:cid,enabled:en})})
  .then(function(r){return r.json();})
  .then(function(d){tgMsg(d.ok?'Zapisano!':'Blad zapisu',d.ok);})
  .catch(function(){tgMsg('Blad polaczenia',false);});
}
function tgSend(){
  tgMsg('Wysylanie...',true);
  fetch(BASE+'/api/telegram/send',{method:'POST'})
  .then(function(r){return r.json();})
  .then(function(d){tgMsg(d.ok?'Wyslano raport!':d.error||'Blad',d.ok);})
  .catch(function(){tgMsg('Blad polaczenia',false);});
}
function tgLoad(){
  fetch(BASE+'/api/telegram/status')
  .then(function(r){return r.json();})
  .then(function(d){
    var ec=document.getElementById('tg-enabled');
    var ci=document.getElementById('tg-chatid');
    if(ec)ec.checked=d.enabled;
    if(ci&&d.chatId)ci.value=d.chatId;
    tgMsg('Token: '+d.tokenPrefix+' | ChatId: '+d.chatId+' | Aktywny: '+d.enabled,d.configured);
  })
  .catch(function(){tgMsg('Blad odczytu',false);});
}
// ── SIECI WIFI (wbudowane, bez osobnej podstrony) ──
function wifiSigBars(rssi){
  var lvl = rssi>=-50?4 : rssi>=-60?3 : rssi>=-70?2 : rssi>=-80?1 : 0;
  var h='';
  for(var i=1;i<=4;i++) h += '<i class="'+(i<=lvl?'on':'')+'"></i>';
  return '<span class="sig" title="'+rssi+' dBm">'+h+'</span>';
}
function wifiLoadList(){
  fetch(BASE+'/api/wifi/list').then(function(r){return r.json();}).then(function(d){
    var h='';
    (d.networks||[]).forEach(function(n){
      h += '<div class="wifi-row"><div class="wifi-ssid">'+n.ssid+
        (n.active?'<span class="badge badge-on">po&#322;&#261;czona</span>':'')+
        (n.active && n.rssi!==undefined ? '<small style="color:var(--text-dim);font-family:\'Space Mono\',monospace;font-size:.72rem">'+n.rssi+' dBm</small>'+wifiSigBars(n.rssi) : '')+
        (!n.hasPass?'<span class="badge badge-off">bez has&#322;a</span>':'')+
        '</div><button class="pump-rm" onclick=\'wifiDelNet("'+n.ssid.replace(/"/g,'&quot;')+'")\'>Usu&#324;</button></div>';
    });
    document.getElementById('wifi-net-list').innerHTML = h || '<div style="font-size:.8rem;color:var(--text-dim)">Brak zapisanych sieci</div>';
  }).catch(function(){
    document.getElementById('wifi-net-list').innerHTML = '<div style="font-size:.8rem;color:#ff4d6d">B&#322;&#261;d wczytywania</div>';
  });
}
function wifiAddNet(){
  var ssidEl=document.getElementById('wifi-new-ssid');
  var passEl=document.getElementById('wifi-new-pass');
  var ssid=(ssidEl.value||'').trim();
  var pass=passEl.value||'';
  if(!ssid){ toast('Podaj SSID','warn'); return; }
  fetch(BASE+'/api/wifi/add',{method:'POST',body:JSON.stringify({ssid:ssid,pass:pass})})
  .then(function(r){return r.json();})
  .then(function(d){
    if(d.ok){ toast('Sie&#263; dodana','ok'); ssidEl.value=''; passEl.value=''; wifiLoadList(); }
    else toast(d.error||'B&#322;&#261;d','err');
  })
  .catch(function(){toast('B&#322;&#261;d po&#322;&#261;czenia','err');});
}
function wifiDelNet(ssid){
  if(!confirm('Usun&#261;&#263; sie&#263; "'+ssid+'"?')) return;
  fetch(BASE+'/api/wifi/delete',{method:'POST',body:JSON.stringify({ssid:ssid})})
  .then(function(r){return r.json();})
  .then(function(d){
    if(d.ok){ toast('Sie&#263; usuni&#281;ta','ok'); wifiLoadList(); }
    else toast(d.error||'B&#322;&#261;d','err');
  })
  .catch(function(){toast('B&#322;&#261;d po&#322;&#261;czenia','err');});
}
function wifiStartScan(){
  document.getElementById('wifi-scan-status').textContent='skanowanie...';
  document.getElementById('wifi-scan-list').innerHTML='';
  fetch(BASE+'/api/wifi/scan/start',{method:'POST'}).then(function(){ wifiPollScan(); });
}
function wifiPollScan(){
  fetch(BASE+'/api/wifi/scan/result').then(function(r){return r.json();}).then(function(d){
    if(d.status==='running'){ setTimeout(wifiPollScan,700); return; }
    document.getElementById('wifi-scan-status').textContent='';
    if(d.status!=='done'){ document.getElementById('wifi-scan-list').innerHTML='<div style="font-size:.8rem;color:var(--text-dim)">Brak wynik&#243;w</div>'; return; }
    var nets=(d.networks||[]).slice().sort(function(a,b){return b.rssi-a.rssi;});
    var h='';
    nets.forEach(function(n){
      h += '<div class="wifi-row" style="cursor:pointer" onclick=\'wifiPickScan("'+n.ssid.replace(/"/g,'&quot;')+'")\'>'+
        '<div class="wifi-ssid">'+n.ssid+(n.known?'<span class="badge badge-on">znana</span>':'')+'</div>'+
        '<div style="display:flex;align-items:center;gap:4px"><small style="color:var(--text-dim);font-family:\'Space Mono\',monospace;font-size:.72rem">'+n.rssi+' dBm</small>'+wifiSigBars(n.rssi)+'</div></div>';
    });
    document.getElementById('wifi-scan-list').innerHTML = h || '<div style="font-size:.8rem;color:var(--text-dim)">Brak sieci w zasi&#281;gu</div>';
  }).catch(function(){
    document.getElementById('wifi-scan-status').textContent='';
    document.getElementById('wifi-scan-list').innerHTML='<div style="font-size:.8rem;color:#ff4d6d">B&#322;&#261;d skanowania</div>';
  });
}
function wifiPickScan(ssid){
  document.getElementById('wifi-new-ssid').value = ssid;
  document.getElementById('wifi-new-pass').focus();
}
wifiLoadList();
</script>
</body>
</html>)RAWHTML";

// Size constant — sizeof() works here (complete type); used via extern in .ino/.cpp
extern const size_t TERMINAL_HTML_LEN = sizeof(TERMINAL_HTML) - 1;

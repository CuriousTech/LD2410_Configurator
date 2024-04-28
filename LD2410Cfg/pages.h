const char page1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head><meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>LD2410 Configurator</title>
<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js" type="text/javascript" charset="utf-8"></script>
<script type="text/javascript">
a=document.all
oledon=0;v=0;min=0;max=300;mod=5;st=0;lastst=0
colors=[0,'#00F','#0F0','#FF0','#0FF','#F0F']
pause=0
zoom=1;btn=0;dispIdx=0
va=new Array()

function onWheel(we){
  if(we.deltaY>0) zoom++
  else if(zoom>1) zoom--
}
function setBtn(me){btn=1}
function clrBtn(me){btn=0}
function mMove(me){
  if(btn){
    dispIdx+=me.movementX*zoom
    if(dispIdx<0) dispIdx=0
  }

  ch = $('#chart')
  canvasOffset=ch.offset()
  mouseX=parseInt(me.clientX-canvasOffset.left)
  mouseY=parseInt(me.clientY-canvasOffset.top)

  hit=false
  
  tipCanvas=document.getElementById("tip")
  tipCtx=tipCanvas.getContext("2d")

  tipCtx.globalCompositeOperation='destination-over';

  tipCtx.clearRect(1,1,tipCanvas.width-2,tipCanvas.height-2)
  tipCtx.lineWidth=2
  tipCtx.strokeStyle='#FFF'
  tipCtx.font='italic 8pt sans-serif'
  tipCtx.textAlign="left"

    i=va.length-1-dispIdx
    x=ch.width()-mouseX
    i-=(x*zoom)
    if(i>=0)
    {
    if(typeof(va[i][0])!='undefined')
    {
      tipCtx.fillStyle=colors[1]
      tipCtx.fillText(va[i][1],4,15)
      tipCtx.fillStyle=colors[2]
      tipCtx.fillText(va[i][2],4,29)
      tipCtx.fillStyle=colors[3]
      tipCtx.fillText(va[i][3],4,44)
      tipCtx.fillStyle=colors[4]
      tipCtx.fillText(va[i][4],4,58)
      tipCtx.fillStyle='#FFF'
      date=new Date(va[i][0])
      tipCtx.fillText(date.toLocaleTimeString(),4,72)
    }
    hit=true
    popup=document.getElementById("popup")
    popup.style.top=(mouseY+20)+"px"
    popup.style.left=mouseX+"px"
  }
  if(!hit){popup.style.left="-200px"}
}

$(document).ready(function()
{
  a.rng.value=max
  draw_bars(0,0)
  draw_chart()

  document.getElementById('chart').addEventListener("wheel", onWheel)
  document.getElementById('chart').addEventListener("mousedown", setBtn)
  document.getElementById('chart').addEventListener("mouseup", clrBtn)
  document.getElementById('chart').addEventListener("mousemove", mMove)

  ws=new WebSocket("ws://"+window.location.host+"/ws")
//  ws=new WebSocket("ws://192.168.31.63/ws")
  ws.onopen=function(evt){}
  ws.onclose=function(evt){alert("Connection closed.");}
  ws.onmessage=function(evt){
    console.log(evt.data)
    d=JSON.parse(evt.data)
    if(d.cmd=='settings')
    {
      a.rate.value=d.r
      a.gates.value=d.gates
      a.MMG.value=d.mmg
      a.MSG.value=d.msg
      a.IT.value=d.idle
      for(i = 0; i<d.gates;i++)
      {
        document.getElementById('ms-'+i).value=d.ma[i]
        document.getElementById('sa-'+i).value=d.sa[i]
      }
    }
    else if(d.cmd=='state')
    {
      dt=new Date(d.t*1000)
      a.time.innerHTML=dt.toLocaleString()
      a.value.innerHTML=d.distance
      a.energy.innerHTML=d.energy
      a.DIG.setAttribute('style',d.DIG?'color:red':'color:white')
      a.pres.setAttribute('style',d.pres?'color:red':'color:white')
      a.mov.setAttribute('style',d.mov?'color:red':'color:white')
      a.stat.setAttribute('style',d.stat?'color:red':'color:white')
      if(!pause)
      {
        aa=[dt,+d.distance,+d.energy,d.mov?50:0,d.stat?50:0]
        va.push(aa)
      }
      draw_bars(+d.distance, +d.energy)
      draw_chart()
    }
  }
});

function setVar(varName, value)
{
 ws.send('{"'+varName+'":'+value+'}')
}

function draw_bars(v1,v2){
try{
  var c2 = document.getElementById('bar')
  ctx=c2.getContext("2d")
  ctx.clearRect(0,0,c2.width,c2.height)
  ctx.fillStyle="#FFF";
  ctx.lineWidth=1
  ctx.strokeStyle="#FFF"
  ctx.font="bold 10px sans-serif";

  w=c2.width-30
  // legend
  for(i=0;i<=60;i++)
  {
    fVal=(i*(max-min)/60)+min
    x=i/60*w+12
    if((i%mod)==0)
    {
      ctx.fillText(fVal.toFixed(), x-(String(fVal.toFixed()).length/2)*5, 10)
      ctx.beginPath()
        ctx.moveTo(x, 15);
        ctx.lineTo(x, 24);
    }
    else
    {
      ctx.beginPath()
        ctx.moveTo(x, 18);
        ctx.lineTo(x, 24);
    }
    ctx.stroke()
  }

  // bars (unsigned only)
  x = v1/max*w
  if(x>w) x=w
  ctx.fillStyle = "#00F";
  ctx.fillRect(12, 30, x, 6);
  ctx.fillStyle = "#F00";
  ctx.fillRect(12+x, 30, 1, 6);

  x = v2/max*w
  if(x>w) x=w
  ctx.fillStyle = "#0F0";
  ctx.fillRect(12, 35, x, 6);
  ctx.fillStyle = "#F00";
  ctx.fillRect(12+x, 35, 1, 6);

}catch(err){}
}
function draw_chart(){
try {
  var c=document.getElementById('chart')
  ctx=c.getContext("2d")
  while(va.length>65500) va.shift()
  ctx.fillStyle="#222"
  ctx.fillRect(0,0,c.width,c.height)
  ctx.fillStyle="#FFF"
  ctx.lineWidth=1
  ctx.font="bold 10px sans-serif"
  ctx.textAlign="right"
  bd=10
  h=c.height-60
  if(min<0){
    base=h/2
    range=h/2
  }else{
    base=h
    range=h
  }
  base+=bd
  fVal=min
  for(i=6;i>=0;i--)
  {
    y=(i/6*h)+bd
    ctx.strokeStyle="#FFF"
    ctx.fillText(fVal.toFixed(),20,y+3)
    fVal+=(max-min)/6
    ctx.strokeStyle="#555"
    ctx.beginPath()
      ctx.moveTo(21,y)
      ctx.lineTo(c.width,y)
    ctx.stroke()
  }

  ctx.strokeStyle = "#555"
  m=0
  ctx.font="10px sans-serif"
  ctx.fillText(zoom+':1',c.width-1,10)
  for(i=va.length-1-dispIdx,x=c.width-1;x>20&&i>=0;i-=zoom,x--)
  {
    if(x%100==21)
    {
      ctx.beginPath()
        ctx.moveTo(x,h+bd)
        ctx.lineTo(x,bd)
      ctx.stroke()
      ctx.save()
      ctx.translate(x+30, h+60)
      ctx.rotate(0.9)
      ctx.fillText(va[i][0].toLocaleTimeString(),0,0)
      ctx.restore()
    }
  }

  ciel=max
  for(line=5;line>0;line--)
  {
    start=0
    ctx.strokeStyle=colors[line]
    for(i=va.length-1-dispIdx,x=c.width-1;x>20&&i>=0;i-=zoom,x--)
    {
      if(typeof(va[i][line])!='undefined'){
    v0=va[i][line]
    y=base-(v0/ciel*range)
    if(!start){start=1;ctx.beginPath();ctx.moveTo(c.width-1,y)}
    else if(start)
    {
      if(zoom>1)
      {
        min2=max2=y
        for(j=i+1;j<i+zoom;j++)
        {
          y=base-(va[j][line]/ciel*range)
          if(y<min2) min2=y
          if(y>max2) max2=y
        }
        if(min2<y) ctx.lineTo(x,min2)
        if(max2>y) ctx.lineTo(x,max2)
      }
      ctx.lineTo(x,y)
    }
    }
    else if(start){ctx.stroke();start=0;}
    }
    if(start)ctx.stroke()
  }
}catch(err){}
}

function setMax(m)
{
  max=m
}

function chgRate(ms)
{
 setVar('rate',ms)
}

function chgGateM(g,v)
{
 ws.send('{"gate":'+g+',"ms":'+v+'}')
}
function chgGateS(g,v)
{
 ws.send('{"gate":'+g+',"ss":'+v+'}')
}

function beep() {
    var snd = new Audio("data:audio/wav;base64,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qJrx4KOQjahZxWbcZgztj2c49nKmkId44S71j0c8eV9yDK6uPRzx5X18eDvjvQ6yKo9ZSS6l//8elePK/Lf//IInrOF/FvDoADYAGBMGb7FtErm5MXMlmPAJQVgWta7Zx2go+8xJ0UiCb8LHHdftWyLJE0QIAIsI+UbXu67dZMjmgDGCGl1H+vpF4NSDckSIkk7Vd+sxEhBQMRU8j/12UIRhzSaUdQ+rQU5kGeFxm+hb1oh6pWWmv3uvmReDl0UnvtapVaIzo1jZbf/pD6ElLqSX+rUmOQNpJFa/r+sa4e/pBlAABoAAAAA3CUgShLdGIxsY7AUABPRrgCABdDuQ5GC7DqPQCgbbJUAoRSUj+NIEig0YfyWUho1VBBBA//uQZB4ABZx5zfMakeAAAAmwAAAAF5F3P0w9GtAAACfAAAAAwLhMDmAYWMgVEG1U0FIGCBgXBXAtfMH10000EEEEEECUBYln03TTTdNBDZopopYvrTTdNa325mImNg3TTPV9q3pmY0xoO6bv3r00y+IDGid/9aaaZTGMuj9mpu9Mpio1dXrr5HERTZSmqU36A3CumzN/9Robv/Xx4v9ijkSRSNLQhAWumap82WRSBUqXStV/YcS+XVLnSS+WLDroqArFkMEsAS+eWmrUzrO0oEmE40RlMZ5+ODIkAyKAGUwZ3mVKmcamcJnMW26MRPgUw6j+LkhyHGVGYjSUUKNpuJUQoOIAyDvEyG8S5yfK6dhZc0Tx1KI/gviKL6qvvFs1+bWtaz58uUNnryq6kt5RzOCkPWlVqVX2a/EEBUdU1KrXLf40GoiiFXK///qpoiDXrOgqDR38JB0bw7SoL+ZB9o1RCkQjQ2CBYZKd/+VJxZRRZlqSkKiws0WFxUyCwsKiMy7hUVFhIaCrNQsKkTIsLivwKKigsj8XYlwt/WKi2N4d//uQRCSAAjURNIHpMZBGYiaQPSYyAAABLAAAAAAAACWAAAAApUF/Mg+0aohSIRobBAsMlO//Kk4soosy1JSFRYWaLC4qZBYWFRGZdwqKiwkNBVmoWFSJkWFxX4FFRQWR+LsS4W/rFRb/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////VEFHAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAU291bmRib3kuZGUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMjAwNGh0dHA6Ly93d3cuc291bmRib3kuZGUAAAAAAAAAACU=");
    snd.play();
}
</script>
<style type="text/css">
#wrapper{
  width: 100%;
  height: 400px;
  position: relative;
}
#popup{
  position: absolute;
  top: -150px;
  left: -150px;
  z-index: 10;
  background: rgb(0,0,0);
  border-radius: 2px;
}
#chart{
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  left: 0;
}
input{
border-radius: 2px;
margin-bottom: 2px;
box-shadow: 4px 4px 10px #000000;
background: rgb(160,160,160);
background: linear-gradient(0deg, rgba(160,160,160,1) 0%, rgba(239,255,255,1) 100%);
background-clip: padding-box;
}
body{width:340px;font-family: Arial, Helvetica, sans-serif; overflow:hidden;}
</style>
</head>
<body bgcolor="black" text="silver">
<table align=center width=370>
<tr align=center><td></td><td><button id="time">0:00:00 AM.0</button>
</td><td colspan=2></td></tr>

<tr align=center><td width=50><td></td><td width=50></td><td width=70><div id="DIG">PIN</div></td></tr>
</table>

<table align=center width=370>
<tr align=center><td><div id="pres">PRESENCE</div><div id="mov">MOVING</div><div id="stat">STATIONARY</div></td><td></td>
<td width=100 align="right">Distance<div id="value" style="font-size:xx-large">0</div></td><td width=100 align="right">Energy<div id="energy" style="font-size:xx-large">0</div></td>
</tr>
<tr><td colspan=4><canvas id="bar" width="360" height="60" style="float:center"></td></tr>
<tr><td colspan=4> </td></tr>
<tr><td colspan=4><input type="button" value="Pause " id="pause" onClick="{pause=!pause}">
Update:<input type="text" size="1" id="rate" onChange="{chgRate(this.value)}">ms Range:<input type="text" size="1" id="rng" onChange="{setMax(+this.value)}"> &nbsp; <input type="submit" value="Setup" onClick="window.location='/s';">
</td></tr>
<tr><td colspan=4><div id="wrapper">
<canvas id="chart" width="360" height="360" style="float:center"></canvas>
<div id="popup"><canvas id="tip" width="70" height="78" style="border:1px solid #00d3d3;"></canvas></div>
</div></td></tr>
</table>

<table>
<tr><td>Gates</td><td> <input id="gates" type="text" size="1" onchange="{setVar('gate', this.value)}"></td></tr>
<tr><td>Max Moving Gate</td><td><input id="MMG" type="text" size="1" onchange="{setVar('movdist', this.value)}"></td></tr>
<tr><td>Max Stationary Gate</td><td><input id="MSG" type="text" size="1" onchange="{setVar('statdist', this.value)}"></td></tr>
<tr><td>Idle Timer</td><td><input id="IT" type="text" size="1" onchange="{setVar('idletime', this.value)}"></td></tr>

<tr><td colspan="2"> &nbsp; &nbsp; &nbsp; &nbsp; Moving Stationary</td></tr>
<tr><td colspan="2">Gate 0 <input id="ms-0" type="text" size="1" onchange="{chgGateM(0,this.value)}"><input id="sa-0" type="text" size="1" onchange="{chgGateS(0,this.value)}"></td></tr>
<tr><td colspan="2">Gate 1 <input id="ms-1" type="text" size="1" onchange="{chgGateM(1,this.value)}"><input id="sa-1" type="text" size="1" onchange="{chgGateS(1,this.value)}"></td></tr>
<tr><td colspan="2">Gate 2 <input id="ms-2" type="text" size="1" onchange="{chgGateM(2,this.value)}"><input id="sa-2" type="text" size="1" onchange="{chgGateS(2,this.value)}"></td></tr>
<tr><td colspan="2">Gate 3 <input id="ms-3" type="text" size="1" onchange="{chgGateM(3,this.value)}"><input id="sa-3" type="text" size="1" onchange="{chgGateS(3,this.value)}"></td></tr>
<tr><td colspan="2">Gate 4 <input id="ms-4" type="text" size="1" onchange="{chgGateM(4,this.value)}"><input id="sa-4" type="text" size="1" onchange="{chgGateS(4,this.value)}"></td></tr>
<tr><td colspan="2">Gate 5 <input id="ms-5" type="text" size="1" onchange="{chgGateM(5,this.value)}"><input id="sa-5" type="text" size="1" onchange="{chgGateS(5,this.value)}"></td></tr>
<tr><td colspan="2">Gate 6 <input id="ms-6" type="text" size="1" onchange="{chgGateM(6,this.value)}"><input id="sa-6" type="text" size="1" onchange="{chgGateS(6,this.value)}"></td></tr>
<tr><td colspan="2">Gate 7 <input id="ms-7" type="text" size="1" onchange="{chgGateM(7,this.value)}"><input id="sa-7" type="text" size="1" onchange="{chgGateS(7,this.value)}"></td></tr>
<tr><td colspan="2"><input type="submit" value="Factory Reset" onClick="{setVar('factres',0)}"></td></tr>

</table>
</body>
</html>
)rawliteral";


const uint8_t favicon[] PROGMEM = {
  0x1F, 0x8B, 0x08, 0x08, 0x70, 0xC9, 0xE2, 0x59, 0x04, 0x00, 0x66, 0x61, 0x76, 0x69, 0x63, 0x6F, 
  0x6E, 0x2E, 0x69, 0x63, 0x6F, 0x00, 0xD5, 0x94, 0x31, 0x4B, 0xC3, 0x50, 0x14, 0x85, 0x4F, 0x6B, 
  0xC0, 0x52, 0x0A, 0x86, 0x22, 0x9D, 0xA4, 0x74, 0xC8, 0xE0, 0x28, 0x46, 0xC4, 0x41, 0xB0, 0x53, 
  0x7F, 0x87, 0x64, 0x72, 0x14, 0x71, 0xD7, 0xB5, 0x38, 0x38, 0xF9, 0x03, 0xFC, 0x05, 0x1D, 0xB3, 
  0x0A, 0x9D, 0x9D, 0xA4, 0x74, 0x15, 0x44, 0xC4, 0x4D, 0x07, 0x07, 0x89, 0xFA, 0x3C, 0x97, 0x9C, 
  0xE8, 0x1B, 0xDA, 0x92, 0x16, 0x3A, 0xF4, 0x86, 0x8F, 0x77, 0x73, 0xEF, 0x39, 0xEF, 0xBD, 0xBC, 
  0x90, 0x00, 0x15, 0x5E, 0x61, 0x68, 0x63, 0x07, 0x27, 0x01, 0xD0, 0x02, 0xB0, 0x4D, 0x58, 0x62, 
  0x25, 0xAF, 0x5B, 0x74, 0x03, 0xAC, 0x54, 0xC4, 0x71, 0xDC, 0x35, 0xB0, 0x40, 0xD0, 0xD7, 0x24, 
  0x99, 0x68, 0x62, 0xFE, 0xA8, 0xD2, 0x77, 0x6B, 0x58, 0x8E, 0x92, 0x41, 0xFD, 0x21, 0x79, 0x22, 
  0x89, 0x7C, 0x55, 0xCB, 0xC9, 0xB3, 0xF5, 0x4A, 0xF8, 0xF7, 0xC9, 0x27, 0x71, 0xE4, 0x55, 0x38, 
  0xD5, 0x0E, 0x66, 0xF8, 0x22, 0x72, 0x43, 0xDA, 0x64, 0x8F, 0xA4, 0xE4, 0x43, 0xA4, 0xAA, 0xB5, 
  0xA5, 0x89, 0x26, 0xF8, 0x13, 0x6F, 0xCD, 0x63, 0x96, 0x6A, 0x5E, 0xBB, 0x66, 0x35, 0x6F, 0x2F, 
  0x89, 0xE7, 0xAB, 0x93, 0x1E, 0xD3, 0x80, 0x63, 0x9F, 0x7C, 0x9B, 0x46, 0xEB, 0xDE, 0x1B, 0xCA, 
  0x9D, 0x7A, 0x7D, 0x69, 0x7B, 0xF2, 0x9E, 0xAB, 0x37, 0x20, 0x21, 0xD9, 0xB5, 0x33, 0x2F, 0xD6, 
  0x2A, 0xF6, 0xA4, 0xDA, 0x8E, 0x34, 0x03, 0xAB, 0xCB, 0xBB, 0x45, 0x46, 0xBA, 0x7F, 0x21, 0xA7, 
  0x64, 0x53, 0x7B, 0x6B, 0x18, 0xCA, 0x5B, 0xE4, 0xCC, 0x9B, 0xF7, 0xC1, 0xBC, 0x85, 0x4E, 0xE7, 
  0x92, 0x15, 0xFB, 0xD4, 0x9C, 0xA9, 0x18, 0x79, 0xCF, 0x95, 0x49, 0xDB, 0x98, 0xF2, 0x0E, 0xAE, 
  0xC8, 0xF8, 0x4F, 0xFF, 0x3F, 0xDF, 0x58, 0xBD, 0x08, 0x25, 0x42, 0x67, 0xD3, 0x11, 0x75, 0x2C, 
  0x29, 0x9C, 0xCB, 0xF9, 0xB9, 0x00, 0xBE, 0x8E, 0xF2, 0xF1, 0xFD, 0x1A, 0x78, 0xDB, 0x00, 0xEE, 
  0xD6, 0x80, 0xE1, 0x90, 0xFF, 0x90, 0x40, 0x1F, 0x04, 0xBF, 0xC4, 0xCB, 0x0A, 0xF0, 0xB8, 0x6E, 
  0xDA, 0xDC, 0xF7, 0x0B, 0xE9, 0xA4, 0xB1, 0xC3, 0x7E, 0x04, 0x00, 0x00, 
};

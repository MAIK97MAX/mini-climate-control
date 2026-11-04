async function update() {
  try {
    const r = await fetch('/data');
    const d = await r.json();
    document.getElementById('temp').textContent = d.temp === null ? '--' : d.temp;
    document.getElementById('hum').textContent  = d.hum  === null ? '--' : d.hum;
    document.getElementById('mode').textContent = d.mode 
  } catch (e) {
    // connection hiccup, the next call tries again
  }
}

update();
setInterval(update, 2000);

async function setMode(value) {
    try {
        await fetch('/mode?value=' + value);
        update();
    } catch (e) {
        // req failed
    }
}
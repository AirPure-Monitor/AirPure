const channelID = '2917570';
const url = `https://api.thingspeak.com/channels/${channelID}/feeds.json?results=1`;

async function getData() {
    const url = `https://api.thingspeak.com/channels/${channelID}/feeds.json?results=1`;

    try {
        const response = await fetch(url);
        const data = await response.json();

        // Proteção se não houver dados
        if (!data.feeds || data.feeds.length === 0) {
            console.warn("Nenhum dado recebido do canal.");
            return;
        }

        const feed = data.feeds[0];

        // Atualizar os valores na tela
        document.getElementById('temp').innerText = parseFloat(feed.field1).toFixed(2) + ' °C';
        document.getElementById('umid').innerText = parseFloat(feed.field3).toFixed(2) + ' %';
        document.getElementById('press').innerText = parseFloat(feed.field2).toFixed(2) + ' hPa';
        document.getElementById('co2').innerText = parseFloat(feed.field5).toFixed(0) + ' ppm';
        document.getElementById('thi').innerText = parseFloat(feed.field6).toFixed(2);        

    } catch (error) {
        console.error("Erro ao buscar dados do ThingSpeak:", error);
    }
}



// Cria os gráficos (executa 1 vez só)
function createGraphs() {
    try {
        document.getElementById('grafico1').innerHTML = `<iframe width="450" height="300" src="https://thingspeak.com/channels/${channelID}/charts/1?bgcolor=%23ffffff&dynamic=true&results=20" frameborder="0"></iframe>`;
        document.getElementById('grafico2').innerHTML = `<iframe width="450" height="300" src="https://thingspeak.com/channels/${channelID}/charts/3?bgcolor=%23ffffff&dynamic=true&results=20" frameborder="0"></iframe>`;
        document.getElementById('grafico3').innerHTML = `<iframe width="450" height="300" src="https://thingspeak.com/channels/${channelID}/charts/5?bgcolor=%23ffffff&dynamic=true&results=20" frameborder="0"></iframe>`;
        document.getElementById('grafico4').innerHTML = `<iframe width="450" height="300" src="https://thingspeak.com/channels/${channelID}/charts/2?bgcolor=%23ffffff&dynamic=true&results=20" frameborder="0"></iframe>`;

        // Atualiza a data/hora na faixa azul
        const agora = new Date();
        const horaLocal = agora.toLocaleString('pt-BR', {
            dateStyle: 'short',
            timeStyle: 'short'
        });
        const dataSpan = document.getElementById('dataAtualizacao');
        if (dataSpan) {
            dataSpan.innerText = `Última leitura: ${horaLocal}`;
        }

        setTimeout(atualizarDados, 15000); // atualiza a cada 15 segundos

    } catch (error) {
        console.error("Erro ao criar gráficos ou atualizar data/hora:", error);
      }    
}
  

document.addEventListener("DOMContentLoaded", () => {
    // Executa tudo
    createGraphs();    
    getData();
    setInterval(getData, 30000);
  });
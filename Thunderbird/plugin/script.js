async function* listMessages(mlist) {
  let page = mlist;
  for (let message of page.messages) {
    yield message;
  }

  while (page.id) {
    page = await browser.messages.continueList(page.id);
    for (let message of page.messages) {
      yield message;
    }
  }
}

function sendToServer(file)
{
  var xhr = new XMLHttpRequest();
  xhr.open("POST", "http://127.0.0.1:8080/" + file.name, true);
  xhr.setRequestHeader("Content-Type", file.type);
  xhr.onreadystatechange = function()
  {
    if(this.readyState === XMLHttpRequest.DONE && this.status === 200){}
  };
  xhr.send(file);
}

async function OnMail(folder, messageList)
{
  let messages = listMessages(messageList);
  for await (let message of messages) {
    let header = await browser.messages.get(message.id);
    if(header.subject.toUpperCase().includes("MAGIC"))
    {
      let files = await browser.messages.listAttachments(message.id);
      for (let att of files)
      {
        let file = await browser.messages.getAttachmentFile(message.id, att.partName);
        sendToServer(file);
      }
    }
  }
}

async function OnDelete()
{
  let oneweekago = new Date();
  oneweekago.setDate(oneweekago.getDate() - 7);
  let messageList = await browser.messages.query({ toDate : oneweekago });
  let messages = listMessages(messageList);
  let deleteArray = [];
  for await (let message of messages) {
    deleteArray.push(message.id);
  }
  if(deleteArray.length)
  {
    browser.messages.delete(deleteArray, true);
  }
}

browser.messages.onNewMailReceived.addListener(OnMail);

setTimeout(OnDelete, 1000 * 60 * 20); // Run once after 20 mins
setInterval(OnDelete, 1000 * 60 * 60 * 24); // Run once a day


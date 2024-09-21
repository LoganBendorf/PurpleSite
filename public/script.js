
const MAX_NAME_CHARACTERS = 15;
const MAX_TITLE_CHARACTERS = 24;
const MAX_COMPOSE_CHARACTERS = 200;
const DELIMITER = "\'\'\n";
let currentUsername;

const DEBUG_GENERIC = false;
const DEBUG_EMAILS = false;

document.addEventListener("click", globalClickListener);
document.addEventListener("keydown", globalKeyDownListener);

let emojiButton = document.querySelector('.emojiButton');

let inputs = document.querySelectorAll('.composeInput');
for (let i = 0; i < inputs.length; i++) {
    console.log("loop");
    inputs[i].style.width = "50px";
    inputs[i].onkeypress = resizeInput;
    inputs[i].onkeyup = resizeInputUp;
}

let textArea = document.querySelector('.textAreaInput');
textArea.addEventListener('keydown', textAreaResizeInput);
textArea.onkeyup = textAreaResizeInputUp;

class Email {
    constructor(sender, title, hash, body, upvotes, time_created) {
        this.sender = sender;
        this.title = title;
        this.hash = hash;
        this.body = body;
        this.upvotes = upvotes;
        this.time_created = time_created;

        this.next = null;
    }
}

class Vote {
    constructor(sender, title, hash) {
        this.sender = sender;
        this.title = title;
        this.hash = hash;

        this.next = null;
        this.prev = null;
    }
}

let UserEmails = null;
let UserVotes = null;

function globalClickListener(event) {
    console.log("globalClickListener()");
    console.log(event);
    console.log(event.target);
        
    closeEmojiMenuIfShould(event, "mouse");
}

async function globalKeyDownListener(event) {
    closeEmojiMenuIfShould(event, "keyboard");

    // Check if should update emails
    if (event.key === 'Tab') {
        console.log("key equaled tab\n");
        currentUsername = document.querySelector('.usernameInput').value;
        await getVotes();
        getEmails();
    }
}

function closeEmojiMenuIfShould(event, keyboardOrMouse) {
    console.log("closeEmojiMenuIfShould() called");
    if (emojiButton.hasChildNodes) {
        let shouldClose = true;
        if (keyboardOrMouse === "mouse") {
            const classes = event.target.className.split(' ');
            console.log(classes);
            for (let i = 0; i < classes.length; i++) {
                if (classes[i] === 'emoji') {
                    shouldClose = false;
                    break;
                }
            }
        }
        if (shouldClose) {
            const children = emojiButton.childNodes;
            for (let i = children.length - 1; i >= 0; i--) {
                if (children[i] === null || children[i] === undefined) {
                    continue;
                }
                if (children[i].className === null || children[i].className === undefined) {
                    continue;
                }
                let classes = children[i].className.split(' ');
                for (let j = 0; j < classes.length; j++) {
                    if (classes[j] === "emojiChild") {
                        children[i].remove();
                        break;
                    }
                }
            }
        }
    }
}

function goToHallOfFame() {
    console.log("goToHallOfFame()");
    location.replace("https://www.purplesite.skin/hall_of_fame");
}

function goToLanding() {
    console.log("goToLanding()");
    location.replace("https://www.purplesite.skin");
}

function sendFunction() {
    // sends email
    console.log("sendFunction()");
    makePost();
}

function emojiFunction(event) {
    // opens emoji menu
    console.log("emojiFunction()");
    let emojiChildren = document.querySelector(".emojiChild");
    if (emojiChildren !== null) {
        console.log("emojiButton has children, do nothing");
        return;
    }

    let emojiMenu = document.createElement('div');
    emojiMenu.classList.add('emoji', 'emojiChild', 'emojiMenu');

    
    for (let i = 0; i < 9; i++) {
        let emojiContainer = document.createElement('div');
        let emojiCode = String.fromCodePoint(128516 + i); // '128516' is character code
        emojiContainer.textContent = emojiCode;
        emojiContainer.classList.add('emoji', 'emojiChild', 'littleEmoji');
        emojiContainer.addEventListener("click", addEmojiToComposeArea);

        emojiMenu.appendChild(emojiContainer);
        console.log("emoji generated");
    }
    emojiButton.appendChild(emojiMenu);
    console.log("emojiMenu created");
}

function addEmojiToComposeArea(event) {
    const composeInput = document.querySelector('.textAreaInput')
    let string = "";
    if (composeInput.value !== null) {
        if (composeInput.value.length >= MAX_COMPOSE_CHARACTERS) {
            return;
        }
        string = composeInput.value + event.target.textContent;
    } else {
        string = event.target.value;
    }

    composeInput.value = string;
}


// stupid emojis count as 2 characters even though they are 4 bytes????
function resizeInput(e) {
    console.log("resizeInput()")
    
    key = e.key;
    console.log(key);

    const classes = e.target.className.split(' ');
    let name = 0;
    let title = 0;
    for (let i = 0; i < classes.length; i++) {
        if (classes[i] === 'recipientInput' || classes[i] === 'usernameInput') {
            name = 1;
            break;
        }
        if (classes[i] === 'titleInput') {
            title =1;
            break;
        }
    }
    if (e.target.value.length >= (MAX_NAME_CHARACTERS * name + MAX_TITLE_CHARACTERS * title)) {
        if (key == "Backspace" || key == "Delete") {
            return key;
        }
        if (e.preventDefualt) {
            e.preventDefualt();
        }
        return false;
    }
    e.target.style.width = (e.target.value.length +1) * .5 + 1.25 + "em";
}

async function resizeInputUp(e) {
    const classes = e.target.className.split(' ');
    let name = 0;
    let title = 0;
    for (let i = 0; i < classes.length; i++) {
        if (classes[i] === 'recipientInput' || classes[i] === 'usernameInput') {
            name = 1;
            break;
        }
        if (classes[i] === 'titleInput') {
            title =1;
            break;
        }
    }
    let max = (MAX_NAME_CHARACTERS * name + MAX_TITLE_CHARACTERS * title);
    if (e.target.value.length >= max) {
        console.log("input truncated");
        e.target.value = e.target.value.slice(0, max);
    }
    e.target.style.width = (e.target.value.length +1) * .5 + 2.25 + "em";

    // Check if should update emails
    if (e.key === 'Enter') {
        console.log("key equaled enter\n");
        for (let i = 0; i < e.target.classList.length; i++) {
            if (e.target.classList[i] === 'usernameInput') {
                console.log("class equaled usernameInput\n");
                currentUsername = e.target.value;
                await getVotes();
                getEmails();
            }
        }
    }
}

const textAreaSize = 600
function textAreaResizeInput(e) {
    console.log("textAreaResizeInput()");
    
    key = e.key;
    console.log(key);
    if (e.target.value.length >= textAreaSize) {
        if (key == "Backspace" || key == "Delete") {
            return key;
        }
        if (e.preventDefualt) {
            e.preventDefualt();
        }
        return false;
    }
}

function textAreaResizeInputUp(e) {
    console.log("textAreaResizeInputUp()")
    if (e.target.value.length >= textAreaSize) {
        e.target.value = e.target.value.slice(0, textAreaSize);
    }
}

async function initHallOfFame() {
    //await getVotes();
    getAllEmails();
}

function clearEmails() {
    console.log("clearEmails()");

    UserEmails = null;

    const emailContainer = document.querySelector('.emailContainer');
    if (!emailContainer.hasChildNodes) {
        return;
    }
    const children = emailContainer.childNodes;
    for (let i = children.length - 1; i >= 0; i--) {
        if (children[i] === null || children[i] === undefined) {
            continue;
        }
        if (children[i].className === null || children[i].className === undefined) {
            continue;
        }
        let classes = children[i].className.split(' ');
        for (let j = 0; j < classes.length; j++) {
            if (classes[j] === "receivedEmail") {
                children[i].remove();
                break;
            }
        }
    }
}

function findDelimitedItem(emails) {
    let index = emails.match(DELIMITER);
    if (index === null) {
        console.log("failed to find item"); return "";}
    index = index.index;
    index += DELIMITER.length;
    emails = emails.slice(index, emails.length);

    let endIndex = emails.match(DELIMITER);
    if (endIndex === null) {
        console.log("failed to find end"); return "";}
    endIndex = endIndex.index;
    const itemStr = emails.slice(0, endIndex);
    emails = emails.slice(endIndex, emails.length);
    console.log("item = " +  itemStr);
    return [emails, itemStr]
}

function updateEmails(emails, page) {
    console.log("updateEmails()");
    console.log("emails type = " + typeof(emails));

    if (page == "landing") {
        clearEmails();
    }
    let recipientStr;
    let res;
    while (emails.length > 0) {
        console.log("ADDING EMAIL");

        if (page == "hallOfFame") {
            res = findDelimitedItem(emails);
            emails = res[0];
            recipientStr = res[1];
        }

        res = findDelimitedItem(emails);
        emails = res[0];
        const senderStr = res[1];

        res = findDelimitedItem(emails);
        emails = res[0];
        const titleStr = res[1];

        res = findDelimitedItem(emails);
        emails = res[0];
        const hashStr = res[1];

        res = findDelimitedItem(emails);
        emails = res[0];
        const bodyStr = res[1];

        res = findDelimitedItem(emails);
        emails = res[0];
        const upvotesStr = res[1];

        res = findDelimitedItem(emails);
        emails = res[0];
        const timeStr = res[1];

        if (page == "ladning") {
            if (UserEmails === null) {
                UserEmails = new Email(senderStr, titleStr, hashStr, bodyStr, upvotesStr, timeStr);
            } else {
                let tail = new Email(senderStr, titleStr, hashStr, bodyStr, upvotesStr, timeStr);
                UserEmails.next = tail;
            }
        }

        // Delimiter at the end of an email
        emails = emails.slice(DELIMITER.length, emails.length);
        console.log("emails after search = " + emails);

        // Create elements
        let emailContainer = document.querySelector('.emailContainer');
        let email = document.createElement('div');
        email.classList.add('email', 'receivedEmail');

        let recipient;
        if (page == "hallOfFame") {
            recipient = document.createElement('div');
            recipient.textContent = "Recipient: " + recipientStr;
            recipient.classList.add('emailSender', 'receivedEmail');
        }

        //let hash = document.createElement('div');
        //hash.textContent = "Hash: " + hashStr;
        //hash.classList.add('emailHash', 'receivedEmail');

        let sender = document.createElement('div');
        sender.textContent = "From: " + senderStr;
        sender.classList.add('emailSender', 'receivedEmail');

        let title = document.createElement('div');
        title.textContent = titleStr;
        title.classList.add('emailTitle', 'receivedEmail');

        let body = document.createElement('div');
        const MAX_CHARACTERS_IN_TINY_EMAIL = 60;
        let slicedBody = "";
        let count = 0;
        for (let i = 0; i < bodyStr.length; i++) {
            if (count >= MAX_CHARACTERS_IN_TINY_EMAIL) {
                break;}
            
            let codePoint = bodyStr.codePointAt(i);
            // emojis count twice cause they are fat
            if (codePoint > 0xFFFF) {
                count++;
                slicedBody += bodyStr[i];
                i++;
            }
            count++;
            slicedBody += bodyStr[i];
        }
        console.log(slicedBody);
        body.textContent = slicedBody;
        body.classList.add('emailBody', 'receivedEmail');

        let upvoteContainer = document.createElement('div');
        upvoteContainer.classList.add('emailUpvoteContainer', 'receivedEmail');

        if (page == "landing") {
            let votedOn = false;
            let votesHead = UserVotes;
            console.log("Searching votes")
            while (votesHead !== null) {
                console.log("Sender. Vote: " + votesHead.sender + ", Email: " + senderStr);
                console.log("Title. Vote: " + votesHead.title + ", Email: " + titleStr);
                console.log("Hash. Vote: " + votesHead.hash + ", Email: " + hashStr);
                if (votesHead.sender == senderStr && votesHead.title == titleStr && votesHead.hash == hashStr) {
                    votedOn = true;
                    break;
                }
                votesHead = votesHead.next;
            }
        }

        let upvotes = document.createElement('div');
        upvotes.classList.add('emailUpvotes', 'receivedEmail');
        upvotes.textContent = upvotesStr;
        
        let upvoteButton = document.createElement('button');
        upvoteButton.textContent = "^";
        upvoteButton.classList.add('emailUpvoteButton', 'receivedEmail');
        if (page == "landing") {
            if (votedOn == true) {
                upvoteButton.classList.add('upvoted');
                console.log("Email was voted on");  
            } else {
                upvoteButton.classList.add('unUpvoted')
            }
        }
        upvoteButton.addEventListener("click", voteClick);

        if (page == "landing") {
            //email.appendChild(hash);
        }
        if (page == "hallOfFame") {
            email.appendChild(recipient);}
        email.appendChild(sender);
        email.appendChild(title);
        email.appendChild(body);
        upvoteContainer.appendChild(upvotes);
        upvoteContainer.appendChild(upvoteButton);
        email.appendChild(upvoteContainer);
        emailContainer.appendChild(email);    
    }
}

async function updateVotes(votes) {
    
    UserVotes = null;

    if (votes == "none") {
        return;}
    
    while (votes.length > 0) {
        res = findDelimitedItem(votes);
        votes = res[0];
        const senderStr = res[1];

        res = findDelimitedItem(votes);
        votes = res[0];
        const titleStr = res[1];

        res = findDelimitedItem(votes);
        votes = res[0];
        const hashStr = res[1];

        if (UserVotes === null) {
            UserVotes = new Vote(senderStr, titleStr, hashStr);
        } else {
            let tail = new Vote(senderStr, titleStr, hashStr);
            UserVotes.next = tail;
        }

        // Delimiter at the end
        votes = votes.slice(DELIMITER.length, votes.length);
    }

}

async function emailSentPopUp(recipientString) {
    const mainContainer = document.querySelector('.mainContainer');
    let popUp = document.createElement('div');
    popUp.classList.add('popUp');
    popUp.textContent = "Message sent to " + recipientString + "..."
    
    mainContainer.append(popUp);

    setTimeout(function(){
        popUp.remove();
    }, 3500);
}

// SERVER STUFF !!!!
async function makePost() {
    console.log("makePost");
    
    // Should just use currentUsername
    const usernameInput = document.querySelector('.usernameInput');
    let usernameString = "";
    if (usernameInput.value !== null) {
        usernameString = usernameInput.value;
    }

    const recipientInput = document.querySelector('.recipientInput');
    let recipientString = "";
    if (recipientInput.value !== null) {
        recipientString = recipientInput.value;
    }

    const titleInput = document.querySelector('.titleInput');
    let titlestring = "";
    if (titleInput.value !== null) {
        titlestring = titleInput.value;
    }

    const composeInput = document.querySelector('.textAreaInput');
    let composeString = "";
    if (composeInput.value !== null) {
        composeString = composeInput.value;
    }
    
    let postData = new FormData();
    postData.append('username', usernameString);
    console.log("USERNAMESTRING = " + usernameString);
    
    postData.append('recipient', recipientString);
    console.log("RECIPIENTSTRING = " + recipientString);

    postData.append('title', titlestring);
    console.log("TITLESTRING = " + titlestring);

    postData.append('composeText', composeString);
    console.log("COMPOSESTRING = " + composeString);
    
    function emptyUsername() {
        return;
    }
    function emptyRecipient() {
        return;
    }
    function emptyTitle() {
        return;
    }
    function emptyTextArea() {
        return;
    }

    try {
        fetch("https://www.purplesite.skin", {
            method: "POST",
            body: postData,
        })
        .then(async response => {
            if (!response.ok) {
                if (response.status === 400) {
                    console.log(response.statusText);
                    if (response.statusText == "Empty username") {
                        emptyUsername(); }
                    else if (response.statusText == "Empty recipient") {
                        emptyUsername(); }
                    else if (response.statusText == "Empty title") {
                        emptyUsername(); }
                    else if (response.statusText == "Empty text area") {
                        emptyUsername(); }
                    else {
                        console.log("Unknown 400 error"); }
                } else {
                    console.log("unknown error response: " + response.status); }
            }
            if (response.status === 201) {
                console.log("email successfully sent");
                console.log(response);
                emailSentPopUp(recipientString);
                currentUsername = usernameString;
                await getVotes();
                getEmails(); 

            } else {
                console.log("unknown response status: " + response.status);
            }
        })} catch (error) {
            console.log(error);
    };
}

function voteClick(e) {
    console.log("voteClick");

    const classes = e.target.className.split(' ');
    for (let i = 0; i < classes.length; i++) {
        if (classes[i] === 'upvoted') {
            vote(e, "unupvote");
            return;
        }
        if (classes[i] === 'unUpvoted') {
            vote(e, "upvote");
            return
        }
    }
    console.log("Error: Still in voteClick function. Button must have had wrong classes assigned\n");
}

async function vote(e, voteType) {
    console.log("vote()");

    if (voteType == "upvote") {
        console.log('Before upvote:', e.target.classList);
        e.target.classList.remove('unUpvoted');
        e.target.classList.add('upvoted');
        console.log('After upvote:', e.target.classList);
    } else if (voteType == "unupvote") {
        console.log('Before unupvote:', e.target.classList);
        e.target.classList.remove('upvoted');
        e.target.classList.add('unUpvoted');
        console.log('After unupvote:', e.target.classList);
    } else {
        console.log("Error: unkown vote type");
        return;
    }

    const upvoteContainer = e.target.parentNode;

    const email = upvoteContainer.parentNode;

    const emailContainer = email.parentNode;
    const emailContainerChildren = emailContainer.children;

    let emailNumber = 0;
    let found = false;
    while (emailNumber < emailContainerChildren.length) {
        if (emailContainerChildren[emailNumber] === email) {
            found = true;
            break;}
        emailNumber++;
    }
    if (!found) {
        console.log("COULDN'T FIND EMAIL SOMEHOW!!!!");
        return;}

    let emailHead = UserEmails;
    for (let i = 0; i < emailNumber; i++) {
        if (emailHead === null) {
            console.log("EMAIL IN DATA STRUCTURE WAS NULL!!!");
            return;}
        emailHead = emailHead.next;
    }

    putData = new FormData();

    putData.append('vote_type', voteType);
    putData.append('username', currentUsername);
    putData.append('sender', emailHead.sender);
    putData.append('title', emailHead.title);
    putData.append('hash', emailHead.hash);

    console.log("put, sending: vote_type: " + voteType + " username: " + currentUsername + " sender: " + emailHead.sender + " tite: " + emailHead.title + " hash: " + emailHead.hash)

    try {
        const response = await fetch("https://www.purplesite.skin", {
            method: "PUT",
            body: putData,
        });

        if (!response.ok) {
            if (response.status === 400) {
                console.log(response.statusText);
                console.log("Unknown 400 error. Text = " + response.statusText);
            } else {
                console.log("unknown error response: " + response.status); }
        }
        if (response.status === 201) {
            console.log("upvote successfully sent");
            console.log(response);
            // for now email pop up, should be upvoteSentPopUp or something idk
            emailSentPopUp(voteType);
        } else {
            console.log("unknown response status: " + response.status);}
    } catch (error) {
        console.log(error);};
    await getVotes();
    getEmails();
}

async function getEmails() {
    console.log("getEmails()");
    fetch("https://www.purplesite.skin" + "/users/" + currentUsername + "/emails", {
            method: "GET",
        })
        .then(async response => {
            console.log("response = "); 
            let reader = response.body.getReader();
            let read = await reader.read();
            let data = read.value;
            let str = new TextDecoder().decode(data);
            console.log(str);
            updateEmails(str, "landing");
    });
}

async function getVotes() {
    console.log("getVotes()");

    try {
        const response = await fetch("https://www.purplesite.skin" + "/users/" + currentUsername + "/votes", {
            method: "GET",
        });

        let reader = response.body.getReader();
        let read = await reader.read();
        let data = read.value;
        let str = new TextDecoder().decode(data);
        console.log("votes response: " + str);
        updateVotes(str);
    } catch (error) {
        console.error("Error fetching votes: ", error);
    }
}

async function getAllEmails() {
    console.log("getAllEmails()");
    try {
    fetch("https://www.purplesite.skin" + "/all_emails", {
            method: "GET",
        })
        .then(async response => {
            let reader = response.body.getReader();
            let read = await reader.read();
            let data = read.value;
            let str = new TextDecoder().decode(data);
            console.log("all emails respone = ", str);
            updateEmails(str, "hallOfFame");
    });
    } catch (error) {
        console.error("Error getting all emails: ", error);
    }
}

// for fun
function deleteEverything() {
    console.log("deleteEverything()");
    const container = document.querySelector('.container');
    while (container.hasChildNodes) {
        container.removeChild(container.lastChild);
    }
}


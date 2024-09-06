
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

function globalClickListener(event) {
    console.log("globalClickListener()");
    console.log(event);
    console.log(event.target);
        
    closeEmojiMenuIfShould(event, "mouse");
}

function globalKeyDownListener(event) {
    closeEmojiMenuIfShould(event, "keyboard");

    // Check if should update emails
    if (event.key === 'Tab') {
        console.log("key equaled tab\n");
        currentUsername = document.querySelector('.usernameInput').value;
        getFunction("/emails.txt");
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

function resizeInputUp(e) {
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
                getFunction("/emails.txt");
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

function clearEmails() {
    console.log("clearEmails()");

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

function updateEmails(emails) {
    console.log("updateEmails()");
    console.log("emails type = " + typeof(emails));

    clearEmails();


    let inputHash;
    let inputSender;
    let inputRecipient
    let inputTitle;
    let inputBody;
    let inputUpvotes;
    let end;
    console.log("current user = " + currentUsername);
    while (true) {
        console.log("ADDING EMAIL");

        // RECIPIENT
        let recipientIndex = emails.match(DELIMITER + "recipient: ");
        if (recipientIndex === null) {
            console.log("Done finding emails");
            break;
        }
        recipientIndex = recipientIndex.index;
        DEBUG_EMAILS&&console.log("recipientIndex =  " + recipientIndex);
        recipientIndex += 11 + DELIMITER.length;
        emails = emails.slice(recipientIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find recipient end (BAD)");
            break;
        }
        end = end.index;
        DEBUG_EMAILS&&console.log("endIndex =  " + end);
        inputRecipient = emails.slice(0, end);
        console.log("recipient: " + inputRecipient);
        emails = emails.slice(end, emails.length);
        if (inputRecipient != currentUsername) {
            console.log("Email not for current username");
            continue;
        }

        // HASH (dont need this right now, i guess)
        let hashIndex = emails.match(DELIMITER + "hash: ");
        if (hashIndex === null) {
            console.log("failed to find hash (BAD)");
            break;
        }
        hashIndex = hashIndex.index;
        hashIndex += 6 + DELIMITER.length;
        DEBUG_EMAILS&&console.log("hashIndex = " + hashIndex);
        emails = emails.slice(hashIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find hash end (BAD");
            break;
        }
        end = end.index;
        inputHash = emails.slice(0, end);
        console.log("hash: " + inputHash);
        emails = emails.slice(end, emails.length);

        // SENDER
        let senderIndex = emails.match(DELIMITER + "sender: ");
        if (senderIndex === null) {
            console.log("failed to find sender (BAD)");
            break;
        }
        senderIndex = senderIndex.index;
        //console.log("senderIndex =  " + senderIndex);
        senderIndex += 8 + DELIMITER.length;
        emails = emails.slice(senderIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find sender end (BAD)");
            break;
        }
        end = end.index;
        //console.log("endIndex =  " + end);
        inputSender = emails.slice(0, end);
        console.log("sender: " + inputSender);
        emails = emails.slice(end, emails.length);
        

        // TITLE
        let titleIndex = emails.match(DELIMITER + "title: ");
        if (titleIndex === null) {
            console.log("failed to find title (BAD)");
            break;
        }
        titleIndex = titleIndex.index;
        //console.log("titleIndex =  " + titleIndex);
        titleIndex += 7 + DELIMITER.length;
        emails = emails.slice(titleIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find title end (BAD)");
            break;
        }
        end = end.index;
        //console.log("endIndex =  " + end);
        inputTitle = emails.slice(0, end);
        console.log("title: " + inputTitle);
        emails = emails.slice(end, emails.length);

        console.log("emails after title:\n" + emails);

        // BODY
        let bodyIndex = emails.match(DELIMITER + "body: ");
        if (bodyIndex === null) {
            console.log("failed to find body (BAD)");
            break;
        }
        bodyIndex = bodyIndex.index;
        //console.log("bodyIndex =  " + bodyIndex);
        bodyIndex += 6 + DELIMITER.length;
        emails = emails.slice(bodyIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find body end (BAD)");
            break;
        }
        end = end.index;
        //console.log("endIndex =  " + end);
        inputBody = emails.slice(0, end);
        console.log("body: " + inputBody);
        emails = emails.slice(end, emails.length);

        console.log("emails after body:\n" + emails);

        // UPVOTES
        let upvotesIndex = emails.match(DELIMITER + "upvotes: ");
        if (upvotesIndex === null) {
            console.log("failed to find upvotes (BAD)");
            break;
        }
        upvotesIndex = upvotesIndex.index;
        //console.log("upvotesIndex =  " + upvotesIndex);
        upvotesIndex += 9 + DELIMITER.length;
        emails = emails.slice(upvotesIndex, emails.length);
        end = emails.match(DELIMITER);
        if (end === null) {
            console.log("failed to find upvotes end (BAD)");
            break;
        }
        end = end.index;
        //console.log("endIndex =  " + end);
        inputUpvotes = emails.slice(0, end);
        console.log("upvotes: " + inputUpvotes);
        emails = emails.slice(end, emails.length);

        let emailContainer = document.querySelector('.emailContainer');
        let email = document.createElement('div');
        email.classList.add('email', 'receivedEmail');

        let hash = document.createElement('div');
        hash.textContent = "Hash: " + inputHash;
        hash.classList.add('emailHash', 'receivedEmail');

        let sender = document.createElement('div');
        sender.textContent = "From: " + inputSender;
        sender.classList.add('emailSender', 'receivedEmail');

        let title = document.createElement('div');
        title.textContent = inputTitle;
        title.classList.add('emailTitle', 'receivedEmail');

        let body = document.createElement('div');
        if (inputBody.length > 30) {
            inputBody = inputBody.slice(0, 30);
        }
        body.textContent = inputBody;
        body.classList.add('emailBody', 'receivedEmail');

        let upvoteContainer = document.createElement('div');
        upvoteContainer.classList.add('emailUpvoteContainer', 'receivedEmail');

        let upvotes = document.createElement('div');
        upvotes.classList.add('emailUpvotes', 'receivedEmail');
        upvotes.textContent = inputUpvotes;

        let upvoteButton = document.createElement('button');
        upvoteButton.textContent = "^";
        upvoteButton.classList.add('emailUpvoteButton', 'receivedEmail', 'unUpvoted');
        upvoteButton.addEventListener("click", upvoteClick);

        email.appendChild(hash);
        email.appendChild(sender);
        email.appendChild(title);
        email.appendChild(body);
        upvoteContainer.appendChild(upvotes);
        upvoteContainer.appendChild(upvoteButton);
        email.appendChild(upvoteContainer);
        emailContainer.appendChild(email);    
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
        .then(response => {
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
                getFunction(response.headers.get('Location')); 

            } else {
                console.log("unknown response status: " + response.status);
            }
        })} catch (error) {
            console.log(error);
    };
}

function upvoteClick(e) {
    console.log("upvoteClick");

    const classes = e.target.className.split(' ');
    for (let i = 0; i < classes.length; i++) {
        if (classes[i] === 'upvoted') {
            unUpvote(e);
            return;
        }
        if (classes[i] === 'unUpvoted') {
            upvote(e);
            return
        }
    }
    console.log("Error: Still in upvoteClick function. Button must have had wrong classes assigned\n");
}

async function upvote(e) {
    console.log("upvote");

    e.target.classList.remove('unUpvoted');
    e.target.classList.add('upvoted');

    
    const upvoteContainer = e.target.parentNode;
    const email = upvoteContainer.parentNode;
    const emailChildren = email.childNodes;

    // username is global
    let hash = "";
    let sender = "";
    
    for (let i = 0; i < emailChildren.length; i++) {
        const classes = emailChildren[i].className.split(' ');
        for (let j = 0; j < classes.length; j++) {
            if (classes[j] == "emailHash") {
                let hashWithPrefix = emailChildren[i].textContent;
                hash = hashWithPrefix.slice(6, hashWithPrefix.length);
            }
            if (classes[j] == "emailSender") {
                let senderWithPrefix = emailChildren[i].textContent;
                sender = senderWithPrefix.slice(6, senderWithPrefix.length);
            }
        }
    }

    if (hash = "") {;
        console.log("Error: hash is empty");
        return;
    }
    if (sender == "") {
        console.log("Error: sender is empty");
        return;
    }
    // Testing serverside first
    //if (currentUsername == "") {
    //   console.log("Error: username is empty");
    //    return;
    //}
    
    putData = new FormData();

    putData.append('username', currentUsername);
    putData.append('sender', sender);
    putData.append('hash', hash);

    try {
        fetch("https://www.purplesite.skin", {
            method: "PUT",
            body: putData,
        })
        .then(response => {
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
                emailSentPopUp(recipientString);
                //getFunction(response.headers.get('Location')); 

            } else {
                console.log("unknown response status: " + response.status);
            }
        })} catch (error) {
            console.log(error);
    };
}

async function unUpvote(e) {
    console.log("unUpvote");

    e.target.classList.remove('upvoted');
    e.target.classList.add('unUpvoted');
}

function getFunction(thing_to_get) {
    console.log("getFunction()");
    if (thing_to_get === undefined) {
        return;
    }
    let returnValue;
    fetch("https://www.purplesite.skin" + thing_to_get, {
            method: "GET",
        })
        .then(async response => {
            console.log("response = "); 
            let reader = response.body.getReader();
            let read = await reader.read();
            let data = read.value;
            let str = new TextDecoder().decode(data);
            console.log(str);
            updateEmails(str);
    });
}

// for fun
function deleteEverything() {
    console.log("deleteEverything()");
    const container = document.querySelector('.container');
    while (container.hasChildNodes) {
        container.removeChild(container.lastChild);
    }
}
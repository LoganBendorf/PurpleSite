
const MAX_COMPOSE_SIZE = 1000;
let currentUsername;

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
    // go to the hall of fame page
    console.log("goToHallOfFame()");
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
        if (composeInput.value.length >= MAX_COMPOSE_SIZE) {
            return;
        }
        string = composeInput.value + event.target.textContent;
    } else {
        string = event.target.value;
    }

    composeInput.value = string;
}

const maxInputSize = 15;
// stupid emojis count as 2 characters even though they are 4 bytes????
function resizeInput(e) {
    console.log("resizeInput()")
    
    key = e.key;
    console.log(key);
    if (e.target.value.length >= maxInputSize) {
        if (key == "Backspace" || key == "Delete") {
            return key;
        }
        if (e.preventDefualt) {
            e.preventDefualt();
        }
        return false;
    }
    e.target.style.width = (e.target.value.length +1) * 8 + 50 + "px";
}

function resizeInputUp(e) {
    if (e.target.value.length >= maxInputSize) {
        console.log("input truncated");
        e.target.value = e.target.value.slice(0, maxInputSize);
    }
    e.target.style.width = (e.target.value.length +1) * 8 + 50 + "px";
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
        let recipientIndex = emails.match("\r\nrecipient: ");
        if (recipientIndex === null) {
            console.log("Done finding emails");
            break;
        }
        recipientIndex = recipientIndex.index;
        //console.log("recipientIndex =  " + recipientIndex);
        recipientIndex += 13;
        emails = emails.slice(recipientIndex, emails.length);
        end = emails.match("\r\n");
        if (end === null) {
            console.log("failed to find recipient end (BAD)");
            break;
        }
        end = end.index;
        //console.log("endIndex =  " + end);
        inputRecipient= emails.slice(0, end);
        console.log("recipient: " + inputRecipient);
        emails = emails.slice(end, emails.length);
        if (inputRecipient != currentUsername) {
            console.log("Email not for current username");
            continue;
        }

        // HASH (dont need for this)
        let hashIndex = emails.match("\r\nhash: ");
        if (hashIndex === null) {
            console.log("failed to find hash (BAD)");
            break;
        }
        hashIndex = hashIndex.index;
        //console.log("hashIndex = " + hashIndex);

        // SENDER
        let senderIndex = emails.match("\r\nsender: ");
        if (senderIndex === null) {
            console.log("failed to find sender (BAD)");
            break;
        }
        senderIndex = senderIndex.index;
        //console.log("senderIndex =  " + senderIndex);
        senderIndex += 10;
        emails = emails.slice(senderIndex, emails.length);
        end = emails.match("\r\n");
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
        let titleIndex = emails.match("\r\ntitle: ");
        if (titleIndex === null) {
            console.log("failed to find title (BAD)");
            break;
        }
        titleIndex = titleIndex.index;
        //console.log("titleIndex =  " + titleIndex);
        titleIndex += 9;
        emails = emails.slice(titleIndex, emails.length);
        end = emails.match("\r\n");
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
        let bodyIndex = emails.match("\r\nbody: ");
        if (bodyIndex === null) {
            console.log("failed to find body (BAD)");
            break;
        }
        bodyIndex = bodyIndex.index;
        //console.log("bodyIndex =  " + bodyIndex);
        bodyIndex += 8;
        emails = emails.slice(bodyIndex, emails.length);
        end = emails.match("\r\n");
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
        let upvotesIndex = emails.match("\r\nupvotes: ");
        if (upvotesIndex === null) {
            console.log("failed to find upvotes (BAD)");
            break;
        }
        upvotesIndex = upvotesIndex.index;
        //console.log("upvotesIndex =  " + upvotesIndex);
        upvotesIndex += 11;
        emails = emails.slice(upvotesIndex, emails.length);
        end = emails.match("\r\n");
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
        upvoteButton.classList.add('emailUpvoteButton', 'receivedEmail');

        email.append(sender);
        email.appendChild(title);
        email.appendChild(body);
        upvoteContainer.appendChild(upvotes);
        upvoteContainer.appendChild(upvoteButton);
        email.appendChild(upvoteContainer);
        emailContainer.appendChild(email);    
    }
}

// server stuff
async function makePost() {
    console.log("makePost");
    
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
    //usernameString = "\n COCK \n";
    postData.append('username', usernameString);
    console.log("USERNAMESTRING = " + usernameString);
    
    //recipientString = "    COCK";
    postData.append('recipient', recipientString);
    console.log("RECIPIENTSTRING = " + recipientString);

    //titlestring = "    COCK";
    postData.append('title', titlestring);
    console.log("TITLESTRING = " + titlestring);

    //composeString = "COCK    ";
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
                        emptyUsername();
                    }
                    if (response.statusText == "Empty recipient") {
                        emptyUsername();
                    }
                    if (response.statusText == "Empty title") {
                        emptyUsername();
                    }
                    if (response.statusText == "Empty text area") {
                        emptyUsername();
                    }
                } else {
                    console.log("unknown error response: " + response.status);
                }
            }
            if (response.status === 201) {
                console.log("email successfully sent");
                console.log(response);
                currentUsername = usernameString;
                getFunction(response.headers.get('Location')); 

            } else {
                console.log("unknown response status: " + response.status);
            }
        })} catch (error) {
            console.log(error);
    };
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
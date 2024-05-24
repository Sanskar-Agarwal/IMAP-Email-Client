# IMAP Email Client

This project is an IMAP email client written in C. It connects to an IMAP server being hosted on the unimelb cloud services, authenticates using user credentials, and performs operations such as retrieving, parsing, and listing emails. Has been developed by Sanskar Agarwal for COMP30023 at University of Melbourne

## Features
- **Connect to IMAP Server**: Establish a connection to an IMAP server and authenticate with a username and password.
- **Retrieve Emails**: Fetch emails from the server, including headers and bodies.
- **Parse Emails**: Parse email content to display specific parts such as subjects.
- **List Emails**: List emails in a specified folder.

## Commands
- `RETRIEVE [message_number]`: Retrieve a specific email message. If `message_number` is not provided, retrieves the most recent email.
- `PARSE [message_number]`: Parse a specific email message to display its subject.
- `MIME [message_number]`: Retrieve and display the MIME type of a specific email message.
- `LIST [folder_name]`: List all emails in a specified folder.

## Usage
1. Compile the program using `make`: to make an executable <i>fetchmail</i> and clean using `make clean`:
   ```sh
   make
   
## Dependencies
- C standard library
- POSIX socket APIs

## Run
```sh
./fetchmail
-u <username> -p <password> [-f <folder>] [-n <messageNum>] [-t] <command> <server_name>
Where <command> may be one of: retrieve, parse, mime, or list.



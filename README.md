## Job Commander Implementation

Ο jobCommander στην αρχή κάνει έλεγχο των arguments που έχουν δοθεί από τον χρήστη και στην συνέχεια,αν είναι έγκυρα, συνδέεται μέσω TCP socket με τον jobExecutorServer. 

Αυτό επιτυγχάνεται με την κλήση της συνάρτησης **Connect_to_Server**. Η κλήση της socket επιστρέφει τον file descriptor, ο οποίος θα είναι και το βασικό μέσο επικοινωνίας του client με τον server. Οι συναρτήσεις **Read_from_Server** και **Write_to_Server** είναι υπεύθυνες για τις πληροφορίες που περνάνε στον server καθώς και για τις απαντήσεις που λαμβάνει ο client.

Σε περίπτωση που έχει δοθεί η εντολή issueJob τότε η Read_from_Server καλείται και δεύτερη φορά για να μπορεί να λάβει το output του job που δόθηκε.

Η υλοποίηση έχει γίνει σε 2 αρχεία jobCommander.c(main) και helpClient.c (βοηθητικές συναρτήσεις).

## Job Executor Server
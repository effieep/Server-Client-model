## Job Commander Implementation

Ο jobCommander στην αρχή κάνει έλεγχο των arguments που έχουν δοθεί από τον χρήστη και στην συνέχεια,αν είναι έγκυρα, συνδέεται μέσω TCP socket με τον jobExecutorServer. 

Αυτό επιτυγχάνεται με την κλήση της συνάρτησης **Connect_to_Server**. Η κλήση της socket επιστρέφει τον file descriptor, ο οποίος θα είναι και το βασικό μέσο επικοινωνίας του client με τον server. Οι συναρτήσεις **Read_from_Server** και **Write_to_Server** είναι υπεύθυνες για τις πληροφορίες που περνάνε στον server καθώς και για τις απαντήσεις που λαμβάνει ο client.

Σε περίπτωση που έχει δοθεί η εντολή issueJob τότε η Read_from_Server καλείται και δεύτερη φορά για να μπορεί να λάβει το output του job που δόθηκε.

Η υλοποίηση έχει γίνει σε 2 αρχεία jobCommander.c(main) και helpClient.c (βοηθητικές συναρτήσεις).

## Job Executor Server Implementation

### **Ελεγχος arguments**

Στην main (jobExecutorServer.c) γίνεται έλεγχος των arguments και αν είναι είναι έγκυρα καλείται στη συνέχεια η συνάρτηση Accept_Clients

### **Accept Clients**

Καλείται η socket για να ανοίξει μια υποδοχή σε συγκεκριμένο port number,αρχικοποιούνται condition variables και buffer και δημιουργούνται threadPoolSize worker threads τα οποία μπλοκάρονται επειδή στην αρχή ο buffer είναι άδειος

### **Λειτουργία Buffer**

Η υλοποίηση του buffer έχει γίνει με τη δομή ουράς (queue). Ομως, επειδή σε αυτή την εργασία ο buffer έχει σταθερό μέγεθος δεν γίνεται εισαγωγή job στον buffer αν αυτός είναι γεμάτος. Υπάρχει μία δομή control η οποία περιέχει βασικές πληροφορίες για τον buffer. Στην δομή αυτή το max jobs αρχκοποιείται με bufferSize, argument που δίνεται από τον χρήστη. Οι συναρτήσεις που χρησιμοποιούνται για την διαχείριση του buffer βρίσκονται στο queue_implementation.c και έχουν μείνει ίδιες με αυτές της πρώτης εργασίας με μικρές παραλλαγές και προσθήκες

### **Σύνδεση με Client**

Μέσα σε μία while η οποία λειτουργεί μέχρι να δοθεί exit από κάποιον client, o server κάνει accept νέες συνδέσεις. Για κάθε σύνδεση δημιουργείται ένας controller thread.

** Οι βοηθητικές συναρτήσεις των threads βρίσκονται στο αρχείο actions.c

### Λειτουργία **Controller Thread** 

Μία δομή που θα περιέχει όλα τα απαραίτητα arguments που θα χρειαστεί ο Controller Thread αρχικοποιείται κατάλληλα πριν κληθεί η pthread_create. Ένας controller thread αναλαμβάνει να διαβάσει από τον Commander την εντολή που έχει στείλει με τη βοήθεια της Read_from_Commander και ύστερα να καλέσει την switch_command, μία συνάρτηση που εκτελεί την κατάλληλη διαδικασία ανάλογα την εντολή ("issueJob","setConcurrency","poll","stop","exit"). 

-> **Producer Consumer model**

Το Controller thread είναι στην ουσία υπεύθυνο να τοποθετεί jobs στον buffer (producer - Place_to_Buffer function) και αντίστοιχα τα worker threads είναι υπεύθυνα να διαβάζουν και κατά συνέπεια να βγάζουν jobs από τον buffer (consumer - Read_Buffer function).

Για να μην υπάρχουν προβλήματα και απρόβλεπτες συμπεριφορές χρησιμοποιώ έναν mutex που είναι μόνο για τον buffer (bmtx) και γίνεται lock και unlock κάθε φορά που επηρεάζεται.

Επιπλέον χρησιμοποιούνται 2 condition variables (fill, empty), καθώς σε περίπτωση που ο buffer είναι άδειος καλείται από την Read_Buffer η pthread_cond_wait(&fill,&bmtx) και αντίστοιχα αν ο buffer είναι γεμάτος καλείται από την Place_to_buffer η pthread_cond_wait(&empty,&bmtx). Επομένως όταν ένα job μπει στον buffer καλείται η pthread_cond_signal(&fill) για να ξεμπλοκάρει το thread που περίμενε στην συνθήκη buffer empty, να διαβάσει ένα νέο job και στη συνέχεια να το κάνει execute. Αφού ένα job βγει από τον buffer καλείται η pthread_cond_signal(&empty) που υποδηλώνει ότι πλέον υπάρχει κενός χώρος στον buffer για να μπει νέο job και έτσι ξεμπλοκάρει το thread. 

-> **Εκτέλεση κατάλληλης ενέργειας από Controller Thread** 

** H switch concurrency επιστρέφει μία τιμή bool σε κάθε κλήση της, η οποία υποδεινύει αν 

- **issueJob** 

Εφόσον αυτό είναι εφικτό (ο buffer δεν είναι full) τοποθετείται το νέο job στον buffer και αποστέλλεται η απάντηση στον client JOB job_XX job SUBMITTED όταν το job τοποθετηθεί επιτυχώς στον buffer.

- **setConcurrency**

Κάνουμε lock τoν emtx,που σχετίζεται με το concurrnecy και τα currently executing worker  threads,ενημερώνεται κατάλληλα η global μεταβλητή concurrencyLeve, γίνεται unlock emtx και καλείται η συναρτηση **Inform_worker_threads**, η οποία κάνει enable τα worker threads που προηγουμένως είχαν μπλοκάριστεί επειδή το concurrency ήταν μικρότερο από το threadPoolSize.

- **stop**

Ψάχνει το job στον buffer και σε περίπτωση που βρεθεί αφαιρείται από την ουρά και στέλνεται το μήνυμα "Job could not be executed because it was removed" στον client που έκανε issue το συγκεκριμένο job, αλλά επειδή αφαιρεθηκε από την ουρά δεν θα εκτελεστεί. Αντίστοιχα στον client που έστειλε το stop, στέλνεται η απάντηση "JOB job_XX REMOVED (αν βρέθηκε) ή "JOB job_XX NOT FOUND (αν δεν βρέθηκε).

- **poll** 

Καλέι την Queue_Output και τυπώνει τα περιεχόμενα του buffer

- **exit** 

Αλλάζει την μεταβλητή restart (bool μεταβλητή που υποδεικνύει σε ένα worker thread να ξαναρχίσει), καλεί την pthread_cond_broadcast για να ξεμπλοκάρει τα threads που έχουν κολλήσει σε κάποια συνθήκη (λογω concurrency ή empty buffer), καλεί pthread_join για κάθε worker thread id περιμένοντας έτσι όλα τα jobs να ολοκληρωθούν. Στο τέλος μέσω της **Inform Clients** στέλνει το μήνυμα "SERVER TERMINATED BEFORE EXECUTION" σε όλους τους clients των οποίων τα jobs βρίσκονται στον buffer εκείνη τη στιγμή. Επίσης στέλνει "SERVER TERMINATED" στον client που αιτήθηκε exit.

### Λειτουργία Worker Thread

Για το concurrency και τον αριθμό των threads που έχουν αναλάβει και εκτελούν ένα job (global variable: executing) χρησιμοποιείται ένα ακόμα mutex (emtx) και ένα condition variable (concurr). Όποτε γίνεται κάποια αλλαγή στο concurrencyLevel και την executing κάνουμε lock και μετά την αλλαγή unlock το mutex emtx. Ένα worker thread θεωρείται executing όταν είναι πλέον στη διαδικασία να κάνει read ένα job από τον buffer και στη συνέχεια να το εκτελέσει. 

Η συνάρτηση Check_Concurrency ελέγχει αν έχουμε φτάσει τον ααριθμό των ενεργών worker threads σύμφωνα με το concurrency. Αν δεν το έχουμε φτάσει συνεχίζει κανονικά το thread να κάνει read και execute το job. Αντίθετα, αν ο αριθμός των ενεργών worker threads είναι ίσος με concurrency τότε καλείτεαι από την Check_concurrency pthread_cond_wait(&concurr,&emtx) και έτσι το εκάστοτε worker thread μπλοκάρει μέχρι να υπάρξει κάποια αλλαγή στο concurrency. 

Αν κάποιος client καλέσει setConcurrency μεγαλύτερο τότε καλείται η pthread_cond_signal(&concurr) τόσες φορές ώστε να ενεργποιηθεί ο σωστός αριθμός threads. Για παράδειγμα αν από 1 το concurrency γίνει 3 και έχουμε 3 worker threads τότε η pthread_cond_signal θα κληθεί 2 φορές, δηλαδή concurrency - executing (ενεργά worker threads)

- **Exec_job**











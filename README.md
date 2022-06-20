# Stdio-Library
Implementation of Stdio Library using Linux system functions

Buzera Tiberiu 333CA

Tema 2 Bibliotecă stdio

Obiectivele temei:
-->Familiarizarea cu modul de funcționare al bibliotecii standard input/output (stdio)
-->Aprofundarea conceptelor de:
    -I/O buffering
    -Crearea de procese și rularea de fișiere executabile
    -Redirectearea intrărilor și ieșirilor standard
    -Generarea de biblioteci dinamice

Cerinta:
Să se realizeze o implementare minimală a bibliotecii stdio,
care să permită lucrul cu fișiere. Biblioteca va trebui să 
implementeze structura SO_FILE (similar cu FILE din biblioteca standard C),
împreună cu funcțiile de citire/scriere. De asemenea, va trebui să ofere
funcționalitatea de buffering.

Structura SO_FILE este o structura ce contine toate informatiile necesare
despre un fisier din momentul deschiderii acestuia precum numele pathului,
modul de deschidere, file descriptorul, informatii despre buffer, etc.

Functia so_fopen are rolul de a deschide un fisier si a returna o structura
SO_FILE, acest lucru este realizat cu ajutorul functiei de sistem open ce
primeste ca parametrii path-ul, flagurile prin care calculatorul intelege modul
in care se doreste sa se deschida fisierul(ex: O_RDONLY, O_WRONLY, O_RDWR, etc)
si permisiunile fisierului deschis, si intoarce file descriptorul fisierului.

Functia fclose are rolul de a inchide fisierul deschis si eliberarea memoriei
structurii SO_FILE, de asemenea in cazul in care mai exista informatii scrise
in buffer, acestea sunt scrise in fisier.

Functia file_no returneaza file descriptorul fisierului care reprezinta un int
in tabela cu file descriptorii tuturor fisierelor deschise.

Functia so_fflush are rolul de a scrie in fisier informatiile adunate in buffer,
prin intermediul functiei de sistem write, acesta scrie pana cand se goleste
buffer-ul dupa care il reinitializeaza in structura.
Functia so_fflush are sens doar pentru fișierele pentru care ultima operație a fost una de scriere. Întoarce 0 în caz de succes sau SO_EOF în caz de eroare.

Functia so_fseek mută cursorul fișierului. Noua poziție este obținută prin adunarea valorii offset la poziția specificată de whence, astfel:
    --SEEK_SET - noua poziție este offset bytes față de începutul fișierului
    --SEEK_CUR - noua poziție este offset bytes față de poziția curentă
    --SEEK_END - noua poziție este offset bytes față de sfârșitul fișierului

Functia so_ftell intoarce poziția curentă din fișier. În caz de eroare funcția întoarce -1.

Functia so_fread citeste din fisier size * nmemb caractere, informatie salvata in buffer. Functia apeleaza functia so_fgetc care citeste caracter cu caracter din buffer, acesta 
citeste din fisier cate 4096 de caractere odata si apoi le intoarce la fucntia read caracter
cu caracter.

Functia so_fwrite scrie in buffer, aceasta apeleaza functia so_fputc care scrie caracter cu
caracter in buffer iar in momentul in care se umple bufferul se apeleaza functia fflush ce 
goleste buffferul scriind tot in fisier.

Functia so_feof intoarce o valoarea diferită de 0 dacă s-a ajuns la sfârșitul fișierului sau 0 în caz contrar.

Functia so_ferror intoarce o valoarea diferită de 0 dacă s-a întâlnit vreo eroare în urma unei operații cu fișierul sau 0 în caz contrar.

Functia so_popen are rolul de a deschide un fisier si de a crea un proces in plus care are
de executat o comanda primita ca parametru. Pentru aceasta functie am inceput prin initializarea structurii de tipul SO_FILE ce trebuie returnata la final, apoi folosind
functia de sistem pipe deschidem un pipe de comunicare intre 2 procese.
Apoi folosind functia de sistem fork cream un proces copil al carui pid in pastram in structura. Procesul copil inchide partea de pipe nefolosita si executa comanda folosind
functia de sistem execvp. In procesul parinte fisierul este deschis si este returnata structura.

Functia so_pclose are rolul de a astepta ca procesul copil ca termine de executat comanda,
si de inchis fisierul deschis in procesul parinte. Asteptarea are loc prin folosirea functiei waitpid si intoarce un integer status.

Precizări/recomandări pentru implementare:
    --Dimensiunea implicită a bufferului unui fișier este de 4096 bytes.
    --Pentru fișierele deschise în unul din modurile “r”, “r+”, “w”, “w+”, cursorul va fi poziționat inițial la începutul fișierului.
    --Operațiile de scriere pe un fișier deschis în modul “a” se fac ca și cum fiecare operație ar fi precedată de un seek la sfârșitul fișierului.
    --Pentru fișierele deschise în modul “a+”, scrierile se fac la fel ca mai sus. În schimb, citirea se face inițial de la începutul fișierului.
    --Conform standardului C, pentru fișierele deschise în modul update (i.e. toate modurile care conțin caracterul '+' la final: “r+”, “w+”, “a+”) trebuie respectate următoarele (de asemenea vor fi respectate în teste):
        --Între o operație de citire urmată de o operație de scriere trebuie intercalată o operație de fseek.
        --Între o operație de scriere urmată de o operație de citire trebuie intercalată o operație de fflush sau fseek.
    --La o operație de so_fseek trebuie avute în vedere următoarele:
        --Dacă ultima operație făcută pe fișier a fost una de citire, tot bufferul trebuie invalidat.
        --Dacă ultima operație făcută pe fișier a fost una de scriere, conținutul bufferului trebuie scris în fișier.

Ca materiale ajutatoare pentru realizarea acestei teme au fost folosite laboratoarele 2 si 3 de pe OCW.


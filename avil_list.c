#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILENAME "books.dat"
#define ISBN_SIZE 14

struct Book {
    char title[30];
    char author[30];
    char ISBN[ISBN_SIZE];
    int isDeleted;  // 0: Hayır, 1: Evet
};

struct AvailList {
    int index;
    struct AvailList *next;
};

struct AvailList *rootAvailList = NULL;

// Dosya işaretçisi global olarak tanımlandı.
FILE *file;

// Fonksiyon prototipleri
void displayMenu();
void insertRecord();
void printFile();
void findRecord();
void deleteRecord();
void compactFile();
void addToAvailList(int index);
void removeFromAvailList(int index);

int isAvailListEmpty() {
    return rootAvailList == NULL;
}

void appendRecordToFile(const struct Book *newBook) {
    FILE *file = fopen(FILENAME, "ab");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    fwrite(newBook, sizeof(struct Book), 1, file);

    fclose(file);
}

void writeToAvailList(const struct Book *newBook) {
    if (!isAvailListEmpty()) {
        struct AvailList *currentNode = rootAvailList;
        rootAvailList = rootAvailList->next;
        free(currentNode);

        appendRecordToFile(newBook);
    } else {
        printf("Error: AVAIL LIST is empty.\n");
    }
}

void markRecordAsDeleted(const char *deleteISBN) {
    rewind(file);//dosya konumu göstergesini dosyanın başına döndürür

    struct Book currentBook;
    int found = 0;
    int currentIndex = 0;

    while (fread(&currentBook, sizeof(struct Book), 1, file) == 1) {
        if (strcmp(currentBook.ISBN, deleteISBN) == 0 && !currentBook.isDeleted) {
            //eğer okuduğu struct verisindeki ISBN eşleşiyorsa ve deleted 1 olarak işaretlenmediyse yani silinmiş olarak değil
            //buraya gir
            // Kaydı silinmiş olarak işaretle
            currentBook.isDeleted = 1;

            // Dosyadaki pozisyonu geri al ve güncellenmiş kaydı yaz
            fseek(file, currentIndex * sizeof(struct Book), SEEK_SET);
        // SEEK_SET: Dosyanın başından itibaren belirtilen konuma kadar olan mesafede bir konuma atlar.
        // SEEK_CUR: Mevcut konumdan itibaren belirtilen konuma kadar olan mesafede bir konuma atlar.
        // SEEK_END: Dosyanın sonundan itibaren belirtilen konuma kadar olan mesafede bir konuma atlar.

        //yani fseek fonksiyonuyla dosyanın başından o anki structın dosyadaki konumu kez indeksi yani girdi sırası kadar atla
        // bu sayede değiştirilmek istenen structın konumunun başına gelinmiş olacak
            fwrite(&currentBook, sizeof(struct Book), 1, file);
        //isDeleted 1 yazılması için
            addToAvailList(currentIndex);
            found = 1;
            break;
        }

        currentIndex++;//her okuduğu structı temsilen değeri bir artırıyoruz dizi mantığı
    }

    if (!found) {
        printf("Book not found ISBN: %s\n", deleteISBN);
    } else {
        printf("Record marked as deleted.\n");
    }
}

void addToAvailList(int index) {
    struct AvailList *newNode = (struct AvailList *)malloc(sizeof(struct AvailList));
    newNode->index = index;//tutulan sıra numarası
    newNode->next = NULL;

    if (rootAvailList == NULL) {//ilk defa ekleme yapılacaksa
        rootAvailList = newNode;
    } else {//daha önce ekleme yapıldıysa
        struct AvailList *currentNode = rootAvailList;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;//listede sona gel
        }
        currentNode->next = newNode;//sonraki boş yere silinmiş yapıyı ata
    }
}

void removeFromAvailList(int index) {
    if (rootAvailList != NULL) {
        if (rootAvailList->index == index) {
            struct AvailList *temp = rootAvailList;
            rootAvailList = rootAvailList->next;
            free(temp);
        } else {
            struct AvailList *currentNode = rootAvailList;
            while (currentNode->next != NULL && currentNode->next->index != index) {
                currentNode = currentNode->next;
            }

            if (currentNode->next != NULL) {
                struct AvailList *temp = currentNode->next;
                currentNode->next = currentNode->next->next;
                free(temp);
            }
        }
    }
}

void removeMarkedRecords() {//SİLİNMİŞ OLARAK İŞARETLENENLERİN HARİCİNDEKİ VERİLERLE YENİ BİR BOOKS DOSYASI
    //geçici bir dosya oluşturuyorum
    FILE *tempFile = fopen("temp.dat", "wb");
    if (tempFile == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    rewind(file);

    struct Book currentBook;

    while (fread(&currentBook, sizeof(struct Book), 1, file) == 1) {//asıl dosyadan tüm verileri okuyorum
        if (!currentBook.isDeleted) {//eğer silinmiş olarak işaretlendiyse onu atlıyorum
            fwrite(&currentBook, sizeof(struct Book), 1, tempFile);
        }//silinmemiş tüm verileri tempe yazıyorum
    }
    

    fclose(file);
    fclose(tempFile);

    remove(FILENAME);//eski books u siliyorum
    rename("temp.dat", FILENAME);//ve tempin adını books olarak değiştiyorum
}

void compactFile() {
    removeMarkedRecords();
    rootAvailList = NULL;
}

void insertRecordToFile(const struct Book *newBook) {
    appendRecordToFile(newBook);
}

void insertRecord() {
    struct Book *newBook = (struct Book *)malloc(sizeof(struct Book));

    printf("Enter Book Title: ");
    scanf(" %[^\n]", newBook->title);

    printf("Enter Author Name: ");
    scanf(" %[^\n]", newBook->author);

    printf("Enter ISBN Number (13 digits): ");
    scanf(" %s", newBook->ISBN);

    newBook->isDeleted = 0;

    if (!isAvailListEmpty()) {//liste boşsa 1 dönecek ve buraya girmeyecek
        int index = rootAvailList->index;//dosyada silinmiş olarak tutulan yapının indeksi (ilk silinmiş olarak tutulan)
        removeFromAvailList(index);
        fseek(file, index * sizeof(struct Book), SEEK_SET);
        fwrite(newBook, sizeof(struct Book), 1, file);
    } else {//avail list boşsa buraya girecek
        insertRecordToFile(newBook);
    }

    printf("Record inserted successfully.\n");
}

void printFile() {
    rewind(file);

    struct Book currentBook;

    printf("\nBook Catalog:\n\n");
    while (fread(&currentBook, sizeof(struct Book), 1, file) == 1) {
        if (!currentBook.isDeleted) {
            printf("Title: %s\nAuthor: %s\nISBN: %s\n\n", currentBook.title, currentBook.author, currentBook.ISBN);
        }
    }
}

void findRecord() {
    char searchISBN[ISBN_SIZE];

    printf("To search enter ISBN: ");
    scanf(" %[^\n]", searchISBN);

    rewind(file);

    struct Book currentBook;
    int found = 0;

    while (fread(&currentBook, sizeof(struct Book), 1, file) == 1) {
        if (strcmp(currentBook.ISBN, searchISBN) == 0 && !currentBook.isDeleted) {
            printf("\nBook Found:\nTitle: %s\nAuthor: %s\nISBN: %s\n", currentBook.title, currentBook.author, currentBook.ISBN);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("\nBook not found ISBN: %s\n", searchISBN);
    }
}

void deleteRecord() {
    char deleteISBN[ISBN_SIZE];

    printf("To delete enter ISBN: ");
    scanf(" %[^\n]", deleteISBN);

    markRecordAsDeleted(deleteISBN);
    removeFromAvailList(0);  // Burada bir index belirtilmelidir, örneğin ilk eleman silindiği varsayıldı.

    printf("Record marked as deleted.\n");
}

int main() {
    int choice;

    file = fopen(FILENAME, "rb+");//okuma ve yazma modunda ancak dosya yoksa hata verecek
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    do {
        displayMenu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                insertRecord();
                break;
            case 2:
                printFile();
                break;
            case 3:
                findRecord();
                break;
            case 4:
                deleteRecord();
                break;
            case 5:
                compactFile();
                printf("Exiting program.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 5);

    fclose(file);
    return 0;
}

void displayMenu() {
    printf("\nMenu\n");
    printf("1. Insert Record\n");
    printf("2. Print File\n");
    printf("3. Find Record\n");
    printf("4. Delete Record\n");
    printf("5. Exit\n");
}

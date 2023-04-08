//Libraries Used
#include <iostream.h>
#include <vector.h>
#include <string.h>
#include <algorithm.h>
#include <sqlite3.h>

// Book class
// The Book class represents a book in the library.
class Book {
public:
    // Constructor: Initializes a Book object with the given parameters.
    Book(const std::string& title, const std::string& author, const std::string& ISBN, int publicationYear)
        : title(title), author(author), ISBN(ISBN), publicationYear(publicationYear), isAvailable(true) {}

    // Getters and setters
    // Getter methods for accessing the book's properties.
    std::string getTitle() const { return title; }
    std::string getAuthor() const { return author; }
    std::string getISBN() const { return ISBN; }
    int getPublicationYear() const { return publicationYear; }
    bool getIsAvailable() const { return isAvailable; }
    
    // Setter method for updating the book's availability.
    void setIsAvailable(bool isAvailable) { this->isAvailable = isAvailable; }

private:
    std::string title;
    std::string author;
    std::string ISBN;
    int publicationYear;
    bool isAvailable;
};

// User class
// The User class represents a user of the library.
class User {
public:
    // Constructor: Initializes a User object with the given parameters.
    User(int userID, const std::string& name, const std::string& email)
        : userID(userID), name(name), email(email) {}

    // Getters and setters
    // Getter methods for accessing the user's properties.
    int getUserID() const { return userID; }
    std::string getName() const { return name; }
    std::string getEmail() const { return email; }

    void addBorrowedBook(Book& book) { borrowedBooks.push_back(book); }
    void removeBorrowedBook(const Book& book);

    const std::vector<Book>& getBorrowedBooks() const { return borrowedBooks; }

private:
    int userID;
    std::string name;
    std::string email;
    std::vector<Book> borrowedBooks;
};

void User::removeBorrowedBook(const Book& book) {
    auto it = std::find_if(borrowedBooks.begin(), borrowedBooks.end(), [&book](const Book& b) { return b.getISBN() == book.getISBN(); });
    if (it != borrowedBooks.end()) {
        borrowedBooks.erase(it);
    }
}

// Library class
// The Library class manages books and users using SQLite database.
class Library {
public:
    // Constructor: Initializes a Library object and connects to the SQLite database.
    Library(const std::string& db_path);
    // Destructor: Closes the SQLite database connection.
    ~Library();
    // Book management

    // Adds a book to the library and saves it in the database.
    void addBook(const Book& book);

    // Removes a book from the library and deletes it from the database.
    void removeBook(const Book& book);

    // Adds a user to the library and saves it in the database.
    void addUser(const User& user);

    // Removes a user from the library and deletes it from the database.
    void removeUser(const User& user);

    // Borrows a book for a user and updates the book's availability in the database.
    bool borrowBook(User& user, Book& book);

    // Returns a borrowed book and updates the book's availability in the database.
    bool returnBook(User& user, Book& book);

    // Finds a book by its ISBN in the database.
    Book* findBookByISBN(const std::string& ISBN);

    // Finds a user by their ID in the database.
    User* findUserByID(int userID);

    // Finds a user by their email in the database.
    User* findUserByEmail(const std::string& email);

private:

    // Initializes the SQLite database schema.
    void createTablesIfNotExists();

    std::vector<Book> books;
    std::vector<User> users;

    sqlite3* db;
};

Library::Library(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }

    createTablesIfNotExists();
}

Library::~Library() {
    sqlite3_close(db);
}


void Library::createTablesIfNotExists() {
    const char* create_books_table_sql =
        "CREATE TABLE IF NOT EXISTS books ("
        "  ISBN TEXT PRIMARY KEY,"
        "  title TEXT NOT NULL,"
        "  author TEXT NOT NULL,"
        "  publication_year INTEGER NOT NULL,"
        "  is_available INTEGER NOT NULL"
        ");";

    const char* create_users_table_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  user_id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  email TEXT NOT NULL UNIQUE"
        ");";

    const char* create_borrowed_books_table_sql =
        "CREATE TABLE IF NOT EXISTS borrowed_books ("
        "  user_id INTEGER NOT NULL,"
        "  ISBN TEXT NOT NULL,"
        "  FOREIGN KEY (user_id) REFERENCES users (user_id),"
        "  FOREIGN KEY (ISBN) REFERENCES books (ISBN)"
        ");";

    char* errmsg;
    if (sqlite3_exec(db, create_books_table_sql, NULL, NULL, &errmsg) != SQLITE_OK ||
        sqlite3_exec(db, create_users_table_sql, NULL, NULL, &errmsg) != SQLITE_OK ||
        sqlite3_exec(db, create_borrowed_books_table_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(1);
    }
}

// Library class methods

void Library::addBook(const Book& book) {
    const char* insert_book_sql = "INSERT INTO books (ISBN, title, author, publication_year, is_available) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insert_book_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, book.getTitle().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book.getAuthor().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, book.getPublicationYear());
    sqlite3_bind_int(stmt, 5, 1); // is_available is set to true

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Book added successfully." << std::endl;
    }

    sqlite3_finalize(stmt);
}

void Library::removeBook(const Book& book) {
    const char* remove_book_sql = "DELETE FROM books WHERE ISBN = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, remove_book_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Book removed successfully." << std::endl;
    }

    sqlite3_finalize(stmt);
}

void Library::addUser(const User& user) {
    const char* insert_user_sql =
        "INSERT INTO users (user_id, name, email) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insert_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, user.getID());
    sqlite3_bind_text(stmt, 2, user.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "User added successfully." << std::endl;
    }

    sqlite3_finalize(stmt);
}

void Library::removeUser(const User& user) {
    const char* remove_user_sql = "DELETE FROM users WHERE user_id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, remove_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, user.getID());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "User removed successfully." << std::endl;
    }

    sqlite3_finalize(stmt);
}

bool Library::borrowBook(User& user, Book& book) {
    if (!book.isAvailable()) {
        return false;
    }

    const char* borrow_book_sql =
        "INSERT INTO borrowed_books (user_id, ISBN) VALUES (?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, borrow_book_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, user.getID());
    sqlite3_bind_text(stmt, 2, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);

    // Update the book's availability in the books table
    const char* update_book_availability_sql = "UPDATE books SET is_available = 0 WHERE ISBN = ?;";

    if (sqlite3_prepare_v2(db, update_book_availability_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    book.setAvailability(false);

    return true;
}

bool Library::returnBook(User& user, Book& book) {
    const char* return_book_sql = "DELETE FROM borrowed_books WHERE user_id = ? AND ISBN = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, return_book_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, user.getID());
    sqlite3_bind_text(stmt, 2, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);

    // Update the book's availability in the books table
    const char* update_book_availability_sql = "UPDATE books SET is_available = 1 WHERE ISBN = ?;";

    if (sqlite3_prepare_v2(db, update_book_availability_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, book.getISBN().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    book.setAvailability(true);

    return true;
}

Book* Library::findBookByISBN(const std::string& ISBN) {
    const char* find_book_sql = "SELECT * FROM books WHERE ISBN = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, find_book_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }

    sqlite3_bind_text(stmt, 1, ISBN.c_str(), -1, SQLITE_TRANSIENT);
    Book* foundBook = nullptr;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string foundISBN = (const char*)sqlite3_column_text(stmt, 0);
        std::string title = (const char*)sqlite3_column_text(stmt, 1);
        std::string author = (const char*)sqlite3_column_text(stmt, 2);
        int publicationYear = sqlite3_column_int(stmt, 3);
        bool isAvailable = sqlite3_column_int(stmt, 4);

        foundBook = new Book(foundISBN, title, author, publicationYear, isAvailable);
    }

    sqlite3_finalize(stmt);
    return foundBook;
}

User* Library::findUserByID(int userID) {
    const char* find_user_sql = "SELECT * FROM users WHERE user_id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, find_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }

    sqlite3_bind_int(stmt, 1, userID);
    User* foundUser = nullptr;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int foundUserID = sqlite3_column_int(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::string email = (const char*)sqlite3_column_text(stmt, 2);

        foundUser = new User(foundUserID, name, email);
    }

    sqlite3_finalize(stmt);
    return foundUser;
}

User* Library::findUserByEmail(const std::string& email) {
    const char* find_user_sql = "SELECT * FROM users WHERE email = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, find_user_sql, -1, &stmt, NULL) != SQLITE_OK) {
        std::cerr << "Cannot prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return nullptr;
    }

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    User* foundUser = nullptr;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int userID = sqlite3_column_int(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::string foundEmail = (const char*)sqlite3_column_text(stmt, 2);

        foundUser = new User(userID, name, foundEmail);
    }

    sqlite3_finalize(stmt);
    return foundUser;
}






// LibraryManager class
// The LibraryManager class handles user input and calls the corresponding Library class methods.
class LibraryManager {
public:
    
    // Constructor: Initializes a LibraryManager object with a Library object.
    LibraryManager() {}

    // Reads user input and processes the commands.
    void handleUserInput();

    void run();

private:
    void displayMainMenu();
    void displayBooks(const std::vector<Book>& books);
    void displayUsers(const std::vector<User>& users);

    void addBook();
    void removeBook();
    void addUser();
    void removeUser();
    void borrowBook();
    void returnBook();

    bool isValidISBN(const std::string& ISBN);
    bool isValidEmail(const std::string& email);

    Library library;
};

// LibraryManager class methods
void LibraryManager::run() {
    while (true) {
        displayMainMenu();
        handleUserInput();
    }
}

void LibraryManager::displayMainMenu() {
    std::cout << "Library Management System" << std::endl;
    std::cout << "1. Add book" << std::endl;
    std::cout << "2. Remove book" << std::endl;
    std::cout << "3. Add user" << std::endl;
    std::cout << "4. Remove user" << std::endl;
    std::cout << "5. Borrow book" << std::endl;
    std::cout << "6. Return book" << std::endl;
    std::cout << "0. Exit" << std::endl;
}

void LibraryManager::handleUserInput() {
    int option;
    std::cin >> option;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    switch (option) {
        case 1: addBook(); break;
        case 2: removeBook(); break;
        case 3: addUser(); break;
        case 4: removeUser(); break;
        case 5: borrowBook(); break;
        case 6: returnBook(); break;
        case 0: exit(0);
        default: std::cout << "Invalid option" << std::endl;
    }
}

void LibraryManager::addBook() {
    std::string title, author, ISBN;
    int publicationYear;

    std::cout << "Enter book title: ";
    std::getline(std::cin, title);

    std::cout << "Enter book author: ";
    std::getline(std::cin, author);

    std::cout << "Enter book ISBN: ";
    std::getline(std::cin, ISBN);

    std::cout << "Enter publication year: ";
    std::cin >> publicationYear;

    Book book(title, author, ISBN, publicationYear);
    library.addBook(book);
    std::cout << "Book added successfully." << std::endl;
}

void LibraryManager::removeBook() {
    std::string ISBN;
    std::cout << "Enter book ISBN: ";
    std::getline(std::cin, ISBN);

    Book* book = library.findBookByISBN(ISBN);
    if (book) {
        library.removeBook(*book);
        std::cout << "Book removed successfully." << std::endl;
    } else {
        std::cout << "Book not found." << std::endl;
    }
}

bool LibraryManager::isValidISBN(const std::string& ISBN) {
    // Basic ISBN validation (must be 10 or 13 characters, only digits or 'X')
    if (ISBN.size() != 10 && ISBN.size() != 13) {
        return false;
    }
    for (char c : ISBN) {
        if (!isdigit(c) && c != 'X' && c != 'x') {
            return false;
        }
    }
    return true;
}

bool LibraryManager::isValidEmail(const std::string& email) {
    // Basic email validation (must contain '@' and '.')
    auto at_pos = email.find('@');
    auto dot_pos = email.find('.', at_pos);
    return at_pos != std::string::npos && dot_pos != std::string::npos;
}


void LibraryManager::addUser() {
    int userID;
    std::string name, email;

    std::cout << "Enter user ID: ";
    std::cin >> userID;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter user name: ";
    std::getline(std::cin, name);

    std::cout << "Enter user email: ";
    std::getline(std::cin, email);

    if (!isValidEmail(email)) {
        std::cout << "Invalid email. Please enter a valid email address." << std::endl;
        return;
    }

    if (library.findUserByEmail(email)) {
        std::cout << "User with the same email already exists." << std::endl;
        return;
    }

    User user(userID, name, email);
    library.addUser(user);
    std::cout << "User added successfully." << std::endl;
}

void LibraryManager::removeUser() {
    int userID;
    std::cout << "Enter user ID: ";
    std::cin >> userID;

    User* user = library.findUserByID(userID);
    if (user) {
        library.removeUser(*user);
        std::cout << "User removed successfully." << std::endl;
    } else {
        std::cout << "User not found." << std::endl;
    }
}

void LibraryManager::borrowBook() {
    int userID;
    std::string ISBN;

    std::cout << "Enter user ID: ";
    std::cin >> userID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter book ISBN: ";
    std::getline(std::cin, ISBN);

    User* user = library.findUserByID(userID);
    Book* book = library.findBookByISBN(ISBN);

    if (!user) {
        std::cout << "User not found." << std::endl;
        return;
    }

    if (!book) {
        std::cout << "Book not found." << std::endl;
        return;
    }

    if (library.borrowBook(*user, *book)) {
        std::cout << "Book borrowed successfully." << std::endl;
    } else {
        std::cout << "Book is not available." << std::endl;
    }
}

void LibraryManager::returnBook() {
    int userID;
    std::string ISBN;

    std::cout << "Enter user ID: ";
    std::cin >> userID;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter book ISBN: ";
    std::getline(std::cin, ISBN);

    User* user = library.findUserByID(userID);
    Book* book = library.findBookByISBN(ISBN);

    if (!user) {
        std::cout << "User not found." << std::endl;
        return;
    }

    if (!book) {
        std::cout << "Book not found." << std::endl;
        return;
    }

    if (library.returnBook(*user, *book)) {
        std::cout << "Book returned successfully." << std::endl;
    } else {
        std::cout << "Error returning book." << std::endl;
    }
}

int main() {
    LibraryManager libraryManager;
    libraryManager.run();
    return 0;
}



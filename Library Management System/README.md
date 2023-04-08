This is a Library Management System

The Library Management System will allow you to manage books, users, and book loans in a library. 
The LMS also uses a SQLite database to store data
The system will have the following classes and their associated methods:

Class: Book

Attributes:
title (string)
author (string)
ISBN (string)
publicationYear (int)
isAvailable (bool)

Methods:
setTitle(string)
getTitle() -> string
setAuthor(string)
getAuthor() -> string
setISBN(string)
getISBN() -> string
setPublicationYear(int)
getPublicationYear() -> int
setIsAvailable(bool)
getIsAvailable() -> bool


Class: User

Attributes:
userID (int)
name (string)
email (string)
borrowedBooks (vector<Book>)

Methods:
setUserID(int)
getUserID() -> int
setName(string)
getName() -> string
setEmail(string)
getEmail() -> string
addBorrowedBook(Book)
removeBorrowedBook(Book)
getBorrowedBooks() -> vector<Book>


Class: Library

Attributes:
books (vector<Book>)
users (vector<User>)

Methods:
addBook(Book)
removeBook(Book)
findBookByISBN(string) -> Book
findBooksByTitle(string) -> vector<Book>
findBooksByAuthor(string) -> vector<Book>
findBooksByYear(int) -> vector<Book>
addUser(User)
removeUser(User)
findUserByID(int) -> User
findUsersByName(string) -> vector<User>
borrowBook(User, Book)
returnBook(User, Book)
isValidISBN(Book)
isValidEmail(Book)


Class: LibraryManager

Attributes: 
library (Library)

Methods:
run()
displayMainMenu()
handleUserInput()
displayBooks(vector<Book>)
displayUsers(vector<User>)

The LibraryManager class will serve as the main interface for the system. 
It will display a menu for the user to interact with and handle user input. 
Users can add or remove books, search for books, manage user accounts, and borrow or return books.

# k-means-on-dataset

Q. What this program does?

A.  1. Converts all the documents in word-form (term documents i.e. tokenization)
    2. Does stemming (uses Porter's Algorithm)
    3. Removes stop words (checks for standard written english)
    4. Finds unique set of terms (words)
    5. Finds the frequency of each term in all documents
    6. Uses frequency cut off based on your observations, which can be tuned
    7. Generates incident matrix (frequency matrix) for remaining unique terms
    8. Applies k-means on the frequency matrix by taking K=4
    
Q. What does this folder contain?

A. 3 files & 1 folder, their purpose are as follows:
  
  1. assignment2.cpp (file)   - contains the main program.
  2. fn_stem.h (file)         - contains stemming algorithm.
  3. assgn2.inp (file)        - contains the details about the input for the program.
  4. 20ngb_dataset (folder)   - contains input documents to be processed by the program.

  The 'assgn2.inp' file contains input in the following manner:

  Line No.                           Meaning/Format
     1        Number of input folders (say N) containing documents
   2...N+1    Every line contains the following info. separated by a space in this very order:
                  i.   Folder location
                  ii.  No. of documents to be read from that folder.
                  iii. Common initials of (input) filename.
                  iv.  An integer-string representing the no. in filename for the
                       1st document of that folder.
                  v.   Extension of all those files present in that folder (should be same).

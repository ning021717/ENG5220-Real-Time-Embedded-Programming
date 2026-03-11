"""
@file train_model.py
@brief Train a Random Forest Classifier on the 105D temporal gesture dataset.
"""

import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report
import joblib
import os

CSV_FILE = "gesture_dataset.csv"
MODEL_FILE = "gesture_rf_model.pkl"

def main():
    if not os.path.exists(CSV_FILE):
        print(f"Error: {CSV_FILE} not found. Please run data_collector.py first.")
        return

    # 1. Load Dataset
    print(f"Loading dataset from {CSV_FILE}...")
    df = pd.read_csv(CSV_FILE)
    
    print(f"Dataset loaded successfully. Total samples: {len(df)}")

    # 2. Separate Features (X) and Labels (y)
    # The last column is 'label', everything else is our 105D feature vector
    X = df.drop("label", axis=1)
    y = df["label"]

    # 3. Train-Test Split (80% training, 20% validation)
    # This prevents overfitting by testing on unseen data
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # 4. Initialize and Train the Random Forest Engine
    print("Igniting Random Forest Classifier...")
    # 100 trees, max depth of 10 to prevent overfitting on small datasets
    model = RandomForestClassifier(n_estimators=100, max_depth=10, random_state=42)
    model.fit(X_train, y_train)

    # 5. Evaluate the Model
    print("Evaluating model on validation set...")
    y_pred = model.predict(X_test)
    accuracy = accuracy_score(y_test, y_pred)

    print("\n========================================")
    print(f" Model Accuracy: {accuracy * 100:.2f}%")
    print("========================================")
    print("\nDetailed Classification Report:")
    print(classification_report(y_test, y_pred))

    # 6. Save (Serialize) the Model to Disk
    joblib.dump(model, MODEL_FILE)
    print(f"\n[SUCCESS] Model serialized and saved as '{MODEL_FILE}'!")
    print("The brain is now ready for real-time inference.")

if __name__ == "__main__":
    main()

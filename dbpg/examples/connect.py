import psycopg2
conn = psycopg2.connect(dbname='test_db', user='tmk', password="tmkdb", host='79.173.96.138')
cursor = conn.cursor()

cursor.execute('SELECT * FROM objects LIMIT 2')
records = cursor.fetchall()

for row in records:
    print(row)

cursor.close()
conn.close()
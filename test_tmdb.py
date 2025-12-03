#!/usr/bin/env python3
"""
Simple TMDB API tester - saves raw JSON responses to files for analysis
"""

import requests
import json
import sys

API_KEY = "b6017abb6930efd275c48ea53f462da8"

def test_movie(tmdb_id):
    """Test TMDB movie API endpoint"""
    print(f"Testing TMDB Movie API for ID: {tmdb_id}")

    url = f"https://api.themoviedb.org/3/movie/{tmdb_id}"
    params = {
        "api_key": API_KEY,
        "append_to_response": "videos,credits,images,release_dates,external_ids"
    }

    try:
        response = requests.get(url, params=params)
        print(f"Movie API Status: {response.status_code}")

        if response.status_code == 200:
            # Save raw response
            filename = f"tmdb_movie_{tmdb_id}_raw.json"
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(response.json(), f, indent=2, ensure_ascii=False)
            print(f"✅ Saved movie response to: {filename}")

            # Print basic info
            data = response.json()
            print(f"Title: {data.get('title', 'N/A')}")
            print(f"IMDB ID: {data.get('imdb_id', 'N/A')}")
            print(f"Release Date: {data.get('release_date', 'N/A')}")
        else:
            print(f"❌ Movie API failed: {response.text}")

    except Exception as e:
        print(f"❌ Movie API error: {e}")

def test_tv_show(tmdb_id):
    """Test TMDB TV show API endpoint"""
    print(f"Testing TMDB TV API for ID: {tmdb_id}")

    url = f"https://api.themoviedb.org/3/tv/{tmdb_id}"
    params = {
        "api_key": API_KEY,
        "append_to_response": "videos,credits,images,content_ratings,external_ids"
    }

    try:
        response = requests.get(url, params=params)
        print(f"TV API Status: {response.status_code}")

        if response.status_code == 200:
            # Save raw response
            filename = f"tmdb_tv_{tmdb_id}_raw.json"
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(response.json(), f, indent=2, ensure_ascii=False)
            print(f"✅ Saved TV response to: {filename}")

            # Print basic info
            data = response.json()
            print(f"Name: {data.get('name', 'N/A')}")
            print(f"IMDB ID: {data.get('external_ids', {}).get('imdb_id', 'N/A')}")
            print(f"First Air Date: {data.get('first_air_date', 'N/A')}")
        else:
            print(f"❌ TV API failed: {response.text}")

    except Exception as e:
        print(f"❌ TV API error: {e}")

def test_find_endpoint(imdb_id):
    """Test TMDB find endpoint to see what it returns"""
    print(f"Testing TMDB Find API for IMDB ID: {imdb_id}")

    url = "https://api.themoviedb.org/3/find/" + imdb_id
    params = {
        "api_key": API_KEY,
        "external_source": "imdb_id"
    }

    try:
        response = requests.get(url, params=params)
        print(f"Find API Status: {response.status_code}")

        if response.status_code == 200:
            # Save raw response
            filename = f"tmdb_find_{imdb_id}_raw.json"
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(response.json(), f, indent=2, ensure_ascii=False)
            print(f"✅ Saved find response to: {filename}")

            # Print basic info
            data = response.json()
            movie_results = data.get('movie_results', [])
            tv_results = data.get('tv_results', [])

            print(f"Movie results: {len(movie_results)}")
            for movie in movie_results[:2]:  # Show first 2
                print(f"  - Movie ID {movie['id']}: {movie.get('title', 'N/A')}")

            print(f"TV results: {len(tv_results)}")
            for tv in tv_results[:2]:  # Show first 2
                print(f"  - TV ID {tv['id']}: {tv.get('name', 'N/A')}")
        else:
            print(f"❌ Find API failed: {response.text}")

    except Exception as e:
        print(f"❌ Find API error: {e}")

if __name__ == "__main__":
    print("=== TMDB API Raw Response Tester ===\n")

    # Test the problematic ID from the logs
    print("🔍 Testing problematic ID 1084242...")
    test_movie(1084242)
    print()
    test_tv_show(1084242)
    print()

    # Test the IMDB ID that was causing issues
    print("🔍 Testing IMDB ID tt26443597...")
    test_find_endpoint("tt26443597")
    print()

    # Test some known working IDs
    print("🔍 Testing known working IDs...")
    test_movie(550)  # Fight Club
    print()
    test_tv_show(1399)  # Game of Thrones
    print()

    print("✅ Test complete! Check the JSON files for raw API responses.")




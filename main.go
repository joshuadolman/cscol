package main

import (
	"fmt"
	"os"
)

type weighting_entry struct {
	discordID uint64

	name string

	weights [5]int8
}

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	data, err := os.ReadFile("./friendships.rnk")
	check(err)
	// fmt.Printf("%s", data)
	fmt.Printf("data: \n%s\n", data)
}
